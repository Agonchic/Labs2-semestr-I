#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_MEM,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_FORMAT
} status_t;

static status_t safe_char_to_digit(const char ch, const int base, int * const out_digit) {
    if (out_digit == NULL || base < 2 || base > 36) {
        return STATUS_ERR_ARGS;
    }
    int digit = -1;
    if (ch >= '0' && ch <= '9') {
        digit = ch - '0';
    } else if (ch >= 'a' && ch <= 'z') {
        digit = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'Z') {
        digit = ch - 'A' + 10;
    }

    if (digit < 0 || digit >= base) {
        return STATUS_ERR_FORMAT;
    }
    *out_digit = digit;
    return STATUS_OK;
}

static status_t safe_digit_to_char(const int digit, char * const out_ch) {
    if (out_ch == NULL || digit < 0 || digit >= 36) {
        return STATUS_ERR_ARGS;
    }
    if (digit < 10) {
        *out_ch = (char)('0' + digit);
    } else {
        *out_ch = (char)('A' + (digit - 10));
    }
    return STATUS_OK;
}

static status_t safe_strlen(const char * const str, size_t * const out_len) {
    if (str == NULL || out_len == NULL) {
        return STATUS_ERR_ARGS;
    }
    size_t len = 0;
    while (str[len] != '\0') {
        if (len == SIZE_MAX) {
            return STATUS_ERR_OVERFLOW;
        }
        len++;
    }
    *out_len = len;
    return STATUS_OK;
}

static status_t safe_parse_int(const char * const str, int * const out_val) {
    if (str == NULL || out_val == NULL || *str == '\0') {
        return STATUS_ERR_ARGS;
    }

    size_t i = 0;
    if (str[i] == '+') {
        i++;
    } else if (str[i] == '-') {
        return STATUS_ERR_FORMAT;
    }

    if (str[i] == '\0') {
        return STATUS_ERR_FORMAT;
    }

    int result = 0;
    while (str[i] != '\0') {
        if (!isdigit((unsigned char)str[i])) {
            return STATUS_ERR_FORMAT;
        }
        int digit = str[i] - '0';
        if (result > (INT_MAX - digit) / 10) {
            return STATUS_ERR_OVERFLOW;
        }
        result = result * 10 + digit;
        i++;
    }

    *out_val = result;
    return STATUS_OK;
}

static status_t normalize_number(const char * const raw_num, const int base, char ** const out_norm) {
    if (raw_num == NULL || out_norm == NULL || base < 2 || base > 36) {
        return STATUS_ERR_ARGS;
    }

    size_t offset = 0;
    if (raw_num[offset] == '+') {
        offset++;
    } else if (raw_num[offset] == '-') {
        return STATUS_ERR_FORMAT;
    }

    if (raw_num[offset] == '\0') {
        return STATUS_ERR_FORMAT;
    }

    size_t cur = offset;
    while (raw_num[cur] != '\0') {
        int dummy = 0;
        status_t st = safe_char_to_digit(raw_num[cur], base, &dummy);
        if (st != STATUS_OK) {
            return st;
        }
        cur++;
    }

    while (raw_num[offset] == '0' && raw_num[offset + 1] != '\0') {
        offset++;
    }

    size_t len = 0;
    status_t st = safe_strlen(raw_num + offset, &len);
    if (st != STATUS_OK) return st;

    if (len == SIZE_MAX) {
        return STATUS_ERR_OVERFLOW;
    }

    char *norm = (char *)malloc((len + 1) * sizeof(char));
    if (norm == NULL) {
        return STATUS_ERR_MEM;
    }

    for (size_t idx = 0; idx <= len; ++idx) {
        norm[idx] = (raw_num + offset)[idx];
    }

    *out_norm = norm;
    return STATUS_OK;
}

status_t add_column(
    const char * const num1,
    const char * const num2,
    const int base,
    char ** const out_sum
) {
    if (num1 == NULL || num2 == NULL || out_sum == NULL || base < 2 || base > 36) {
        return STATUS_ERR_ARGS;
    }

    char *norm1 = NULL;
    char *norm2 = NULL;
    status_t st = normalize_number(num1, base, &norm1);
    if (st != STATUS_OK) return st;

    st = normalize_number(num2, base, &norm2);
    if (st != STATUS_OK) {
        free(norm1);
        return st;
    }

    size_t len1 = 0;
    size_t len2 = 0;
    st = safe_strlen(norm1, &len1);
    if (st != STATUS_OK) { free(norm1); free(norm2); return st; }
    st = safe_strlen(norm2, &len2);
    if (st != STATUS_OK) { free(norm1); free(norm2); return st; }

    size_t max_len = (len1 > len2) ? len1 : len2;
    if (max_len > SIZE_MAX - 2) {
        free(norm1);
        free(norm2);
        return STATUS_ERR_OVERFLOW;
    }

    size_t res_cap = max_len + 2;
    char *res_buf = (char *)malloc(res_cap * sizeof(char));
    if (res_buf == NULL) {
        free(norm1);
        free(norm2);
        return STATUS_ERR_MEM;
    }

    size_t i = len1;
    size_t j = len2;
    size_t k = 0;
    int carry = 0;

    while (i > 0 || j > 0 || carry > 0) {
        int d1 = 0;
        int d2 = 0;

        if (i > 0) {
            safe_char_to_digit(norm1[i - 1], base, &d1);
            i--;
        }
        if (j > 0) {
            safe_char_to_digit(norm2[j - 1], base, &d2);
            j--;
        }

        int cur_sum = d1 + d2 + carry;
        carry = cur_sum / base;
        int rem = cur_sum % base;

        char ch = '\0';
        st = safe_digit_to_char(rem, &ch);
        if (st != STATUS_OK) {
            free(norm1);
            free(norm2);
            free(res_buf);
            return st;
        }
        res_buf[k++] = ch;
    }

    free(norm1);
    free(norm2);

    for (size_t l = 0; l < k / 2; ++l) {
        char tmp = res_buf[l];
        res_buf[l] = res_buf[k - 1 - l];
        res_buf[k - 1 - l] = tmp;
    }
    res_buf[k] = '\0';

    size_t leading_zeros = 0;
    while (leading_zeros + 1 < k && res_buf[leading_zeros] == '0') {
        leading_zeros++;
    }

    if (leading_zeros > 0) {
        size_t new_len = k - leading_zeros;
        for (size_t idx = 0; idx <= new_len; ++idx) {
            res_buf[idx] = res_buf[idx + leading_zeros];
        }
    }

    *out_sum = res_buf;
    return STATUS_OK;
}
int main(const int argc, const char * const argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <base> [num1] [num2] ... [numN]\n", argv[0]);
        return 1;
    }

    int base = 0;
    status_t st = safe_parse_int(argv[1], &base);
    if (st == STATUS_ERR_OVERFLOW) {
        fprintf(stderr, "Error: Base argument causes integer overflow.\n");
        return 2;
    }
    if (st != STATUS_OK || base < 2 || base > 36) {
        fprintf(stderr, "Error: Base must be an integer between 2 and 36.\n");
        return 2;
    }

    char *accumulator = (char *)malloc(2 * sizeof(char));
    if (accumulator == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 3;
    }
    accumulator[0] = '0';
    accumulator[1] = '\0';

    for (int idx = 2; idx < argc; ++idx) {
        char *next_sum = NULL;
        st = add_column(accumulator, argv[idx], base, &next_sum);
        free(accumulator);

        switch (st) {
            case STATUS_OK:
                accumulator = next_sum;
                break;
            case STATUS_ERR_OVERFLOW:
                fprintf(stderr, "Error: Memory limit/size_t overflow calculating arg '%s'.\n", argv[idx]);
                return 4;
            case STATUS_ERR_MEM:
                fprintf(stderr, "Error: Out of memory while processing arg '%s'.\n", argv[idx]);
                return 5;
            case STATUS_ERR_FORMAT:
                fprintf(stderr, "Error: Invalid character for base %d in arg '%s'.\n", base, argv[idx]);
                return 6;
            case STATUS_ERR_ARGS:
            default:
                fprintf(stderr, "Error: Argument error while processing arg '%s'.\n", argv[idx]);
                return 7;
        }
    }

    printf("Result: %s\n", accumulator);
    free(accumulator);
    return 0;
}







