#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
    OK = 0,
    ERR_ARGS,
    ERR_FLAG,
    ERR_NUM,
    ERR_MEM,
    ERR_OVERFLOW
} status_t;

static int is_digit(const char c) {
    return (c >= '0' && c <= '9');
}

static int is_alpha(const char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

static char to_upper(const char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

status_t get_len(const char * const s, size_t * const res) {
    if (!s || !res) {
        return ERR_ARGS;
    }
    size_t len = 0;
    while (s[len] != '\0') {
        if (len == SIZE_MAX - 1) {
            return ERR_OVERFLOW;
        }
        len++;
    }
    *res = len;
    return OK;
}

status_t reverse_str(const char * const s, char ** const out) {
    if (!s || !out) {
        return ERR_ARGS;
    }
    size_t len = 0;
    status_t st = get_len(s, &len);
    if (st != OK) {
        return st;
    }

    char *buf = (char *)malloc((len + 1) * sizeof(char));
    if (!buf) {
        return ERR_MEM;
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = s[len - 1 - i];
    }
    buf[len] = '\0';
    *out = buf;
    return OK;
}

status_t upper_odd(const char * const s, char ** const out) {
    if (!s || !out) {
        return ERR_ARGS;
    }
    size_t len = 0;
    status_t st = get_len(s, &len);
    if (st != OK) {
        return st;
    }

    char *buf = (char *)malloc((len + 1) * sizeof(char));
    if (!buf) {
        return ERR_MEM;
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = (i % 2 != 0) ? to_upper(s[i]) : s[i];
    }
    buf[len] = '\0';
    *out = buf;
    return OK;
}

status_t reorder_chars(const char * const s, char ** const out) {
    if (!s || !out) {
        return ERR_ARGS;
    }
    size_t len = 0;
    status_t st = get_len(s, &len);
    if (st != OK) {
        return st;
    }

    char *buf = (char *)malloc((len + 1) * sizeof(char));
    if (!buf) {
        return ERR_MEM;
    }

    size_t k = 0;
    for (size_t i = 0; i < len; i++) {
        if (is_digit(s[i])) {
            buf[k++] = s[i];
        }
    }
    for (size_t i = 0; i < len; i++) {
        if (is_alpha(s[i])) {
            buf[k++] = s[i];
        }
    }
    for (size_t i = 0; i < len; i++) {
        if (!is_digit(s[i]) && !is_alpha(s[i])) {
            buf[k++] = s[i];
        }
    }
    buf[len] = '\0';
    *out = buf;
    return OK;
}

status_t str_to_uint(const char * const s, unsigned int * const val) {
    if (!s || !val || s[0] == '\0') {
        return ERR_NUM;
    }
    if (s[0] == '0' && s[1] != '\0') {
        return ERR_NUM;
    }
    unsigned int res = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (!is_digit(s[i])) {
            return ERR_NUM;
        }
        unsigned int d = (unsigned int)(s[i] - '0');
        if (res > (UINT_MAX - d) / 10U) {
            return ERR_OVERFLOW;
        }
        res = res * 10U + d;
    }
    *val = res;
    return OK;
}

status_t concat_rand(const size_t n, const char * const * const arr,
                     const unsigned int seed, char ** const out) {
    if (!arr || !out || n == 0) {
        return ERR_ARGS;
    }
    if (n > SIZE_MAX / sizeof(size_t)) {
        return ERR_OVERFLOW;
    }

    size_t *idx = (size_t *)malloc(n * sizeof(size_t));
    if (!idx) {
        return ERR_MEM;
    }

    size_t total = 0;
    for (size_t i = 0; i < n; i++) {
        if (!arr[i]) {
            free(idx);
            return ERR_ARGS;
        }
        idx[i] = i;
        size_t len = 0;
        status_t st = get_len(arr[i], &len);
        if (st != OK) {
            free(idx);
            return st;
        }
        if (total > SIZE_MAX - 1 - len) {
            free(idx);
            return ERR_OVERFLOW;
        }
        total += len;
    }

    srand(seed);
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        size_t tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }

    char *buf = (char *)malloc((total + 1) * sizeof(char));
    if (!buf) {
        free(idx);
        return ERR_MEM;
    }

    size_t pos = 0;
    for (size_t i = 0; i < n; i++) {
        const char *cur = arr[idx[i]];
        for (size_t k = 0; cur[k] != '\0'; k++) {
            buf[pos++] = cur[k];
        }
    }
    buf[pos] = '\0';

    free(idx);
    *out = buf;
    return OK;
}

int main(int argc, char **argv) {
    if (argc < 3 || !argv) {
        printf("Invalid arguments\n");
        return 1;
    }

    const char *flag = argv[1];
    size_t flen = 0;
    if (get_len(flag, &flen) != OK || flen != 2 || (flag[0] != '-' && flag[0] != '/')) {
        printf("Invalid flag\n");
        return 1;
    }

    status_t st = OK;
    char *res = NULL;

    switch (flag[1]) {
        case 'l': {
            if (argc != 3) {
                printf("Invalid arguments\n");
                return 1;
            }
            size_t len = 0;
            st = get_len(argv[2], &len);
            if (st == OK) {
                printf("%zu\n", len);
            }
            break;
        }
        case 'r':
            if (argc != 3) {
                printf("Invalid arguments\n");
                return 1;
            }
            st = reverse_str(argv[2], &res);
            break;
        case 'u':
            if (argc != 3) {
                printf("Invalid arguments\n");
                return 1;
            }
            st = upper_odd(argv[2], &res);
            break;
        case 'n':
            if (argc != 3) {
                printf("Invalid arguments\n");
                return 1;
            }
            st = reorder_chars(argv[2], &res);
            break;
        case 'c': {
            if (argc < 4) {
                printf("Invalid arguments\n");
                return 1;
            }
            unsigned int seed = 0;
            st = str_to_uint(argv[3], &seed);
            if (st != OK) {
                printf("Invalid seed\n");
                return 1;
            }
            size_t cnt = (size_t)(argc - 3);
            if (cnt > SIZE_MAX / sizeof(char *)) {
                printf("Overflow error\n");
                return 1;
            }
            const char **list = (const char **)malloc(cnt * sizeof(char *));
            if (!list) {
                printf("Memory error\n");
                return 1;
            }
            list[0] = argv[2];
            for (int i = 4; i < argc; i++) {
                list[i - 3] = argv[i];
            }
            st = concat_rand(cnt, list, seed, &res);
            free((void *)list);
            break;
        }
        default:
            printf("Unknown flag\n");
            return 1;
    }

    if (st != OK) {
        printf("Error %d\n", st);
        return 1;
    }

    if (res) {
        printf("%s\n", res);
        free(res);
    }

    return 0;
}
