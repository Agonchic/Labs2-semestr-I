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

typedef struct {
    char *name;
    size_t size;
    size_t importance;
} file_desc_t;

typedef struct {
    char **selected_files;
    size_t count;
    size_t total_size;
    size_t total_importance;
} backup_plan_t;

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

static status_t safe_strdup(const char * const src, char ** const out_dup) {
    if (src == NULL || out_dup == NULL) {
        return STATUS_ERR_ARGS;
    }
    size_t len = 0;
    status_t st = safe_strlen(src, &len);
    if (st != STATUS_OK) return st;

    if (len == SIZE_MAX) return STATUS_ERR_OVERFLOW;

    char *dup = (char *)malloc((len + 1) * sizeof(char));
    if (dup == NULL) {
        return STATUS_ERR_MEM;
    }

    for (size_t i = 0; i <= len; ++i) {
        dup[i] = src[i];
    }
    *out_dup = dup;
    return STATUS_OK;
}

static bool safe_str_equal(const char * const s1, const char * const s2) {
    if (s1 == NULL || s2 == NULL) return false;
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return false;
        i++;
    }
    return s1[i] == s2[i];
}

void free_backup_plan(backup_plan_t * const plan) {
    if (plan == NULL) return;
    if (plan->selected_files != NULL) {
        for (size_t i = 0; i < plan->count; ++i) {
            free(plan->selected_files[i]);
        }
        free(plan->selected_files);
        plan->selected_files = NULL;
    }
    plan->count = 0;
    plan->total_size = 0;
    plan->total_importance = 0;
}

status_t select_files_for_backup(
    const file_desc_t * const files,
    const size_t file_count,
    const size_t capacity,
    backup_plan_t ** const out_plan
) {
    if (out_plan == NULL) {
        return STATUS_ERR_ARGS;
    }
    if (files == NULL && file_count > 0) {
        return STATUS_ERR_ARGS;
    }

    backup_plan_t *plan = (backup_plan_t *)calloc(1, sizeof(backup_plan_t));
    if (plan == NULL) {
        return STATUS_ERR_MEM;
    }

    if (file_count == 0 || capacity == 0) {
        *out_plan = plan;
        return STATUS_OK;
    }

    if (file_count == SIZE_MAX || capacity == SIZE_MAX) {
        free(plan);
        return STATUS_ERR_OVERFLOW;
    }
    size_t rows = file_count + 1;
    size_t cols = capacity + 1;
    if (rows > SIZE_MAX / cols) {
        free(plan);
        return STATUS_ERR_OVERFLOW;
    }
    size_t dp_size = rows * cols;
    if (dp_size > SIZE_MAX / sizeof(size_t)) {
        free(plan);
        return STATUS_ERR_OVERFLOW;
    }

    size_t *dp = (size_t *)calloc(dp_size, sizeof(size_t));
    if (dp == NULL) {
        free(plan);
        return STATUS_ERR_MEM;
    }

    for (size_t i = 1; i <= file_count; ++i) {
        size_t f_size = files[i - 1].size;
        size_t f_imp = files[i - 1].importance;

        for (size_t w = 0; w <= capacity; ++w) {
            size_t without_item = dp[(i - 1) * cols + w];
            if (f_size <= w) {
                size_t with_item = dp[(i - 1) * cols + (w - f_size)] + f_imp;
                dp[i * cols + w] = (with_item > without_item) ? with_item : without_item;
            } else {
                dp[i * cols + w] = without_item;
            }
        }
    }

    bool *chosen = (bool *)calloc(file_count, sizeof(bool));
    if (chosen == NULL) {
        free(dp);
        free(plan);
        return STATUS_ERR_MEM;
    }

    size_t curr_w = capacity;
    size_t total_chosen = 0;
    size_t calc_total_size = 0;

    for (size_t i = file_count; i > 0; --i) {
        if (dp[i * cols + curr_w] != dp[(i - 1) * cols + curr_w]) {
            chosen[i - 1] = true;
            total_chosen++;
            calc_total_size += files[i - 1].size;
            curr_w -= files[i - 1].size;
        }
    }

    plan->total_importance = dp[file_count * cols + capacity];
    plan->total_size = calc_total_size;
    free(dp);

    if (total_chosen > 0) {
        plan->selected_files = (char **)calloc(total_chosen, sizeof(char *));
        if (plan->selected_files == NULL) {
            free(chosen);
            free_backup_plan(plan);
            free(plan);
            return STATUS_ERR_MEM;
        }

        size_t fill_idx = 0;
        for (size_t i = 0; i < file_count; ++i) {
            if (chosen[i]) {
                status_t st = safe_strdup(files[i].name, &plan->selected_files[fill_idx]);
                if (st != STATUS_OK) {
                    free(chosen);
                    free_backup_plan(plan);
                    free(plan);
                    return st;
                }
                fill_idx++;
            }
        }
        plan->count = total_chosen;
    }

    free(chosen);
    *out_plan = plan;
    return STATUS_OK;
}
static status_t safe_read_token(FILE * const stream, char ** const out_token) {
    if (stream == NULL || out_token == NULL) {
        return STATUS_ERR_ARGS;
    }

    int ch = 0;
    while ((ch = fgetc(stream)) != EOF && isspace(ch)) {
    }

    if (ch == EOF) {
        return STATUS_ERR_FORMAT;
    }

    size_t capacity = 16;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity * sizeof(char));
    if (buffer == NULL) {
        return STATUS_ERR_MEM;
    }

    while (ch != EOF && !isspace(ch)) {
        if (length + 1 >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return STATUS_ERR_OVERFLOW;
            }
            size_t new_cap = capacity * 2;
            char *new_buf = (char *)realloc(buffer, new_cap * sizeof(char));
            if (new_buf == NULL) {
                free(buffer);
                return STATUS_ERR_MEM;
            }
            buffer = new_buf;
            capacity = new_cap;
        }
        buffer[length++] = (char)ch;
        ch = fgetc(stream);
    }

    buffer[length] = '\0';
    *out_token = buffer;
    return STATUS_OK;
}

static status_t safe_parse_size_t(const char * const str, size_t * const out_val) {
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

    size_t result = 0;
    while (str[i] != '\0') {
        if (!isdigit((unsigned char)str[i])) {
            return STATUS_ERR_FORMAT;
        }
        size_t digit = (size_t)(str[i] - '0');
        if (result > (SIZE_MAX - digit) / 10) {
            return STATUS_ERR_OVERFLOW;
        }
        result = result * 10 + digit;
        i++;
    }

    *out_val = result;
    return STATUS_OK;
}

static void free_file_descriptors(file_desc_t * const files, const size_t count) {
    if (files == NULL) return;
    for (size_t i = 0; i < count; ++i) {
        if (files[i].name != NULL) {
            free(files[i].name);
        }
    }
    free(files);
}

int main(void) {
    char *n_tok = NULL;
    char *cap_tok = NULL;

    status_t st = safe_read_token(stdin, &n_tok);
    if (st != STATUS_OK) {
        fprintf(stderr, "Diagnostic: Failed to read file count 'n'.\n");
        return 1;
    }
    st = safe_read_token(stdin, &cap_tok);
    if (st != STATUS_OK) {
        free(n_tok);
        fprintf(stderr, "Diagnostic: Failed to read storage capacity.\n");
        return 2;
    }

    size_t n = 0;
    size_t capacity = 0;
    st = safe_parse_size_t(n_tok, &n);
    free(n_tok);
    if (st != STATUS_OK || n == 0) {
        free(cap_tok);
        fprintf(stderr, "Diagnostic: 'n' must be a strictly positive integer.\n");
        return 3;
    }

    st = safe_parse_size_t(cap_tok, &capacity);
    free(cap_tok);
    if (st != STATUS_OK) {
        fprintf(stderr, "Diagnostic: 'capacity' must be a valid non-negative integer.\n");
        return 4;
    }

    if (n > SIZE_MAX / sizeof(file_desc_t)) {
        fprintf(stderr, "Diagnostic: Arithmetic overflow on file buffer allocation.\n");
        return 5;
    }
    file_desc_t *files = (file_desc_t *)calloc(n, sizeof(file_desc_t));
    if (files == NULL) {
        fprintf(stderr, "Diagnostic: Memory allocation failed for files.\n");
        return 6;
    }

    for (size_t i = 0; i < n; ++i) {
        char *name = NULL;
        char *sz_tok = NULL;
        char *imp_tok = NULL;

        st = safe_read_token(stdin, &name);
        if (st == STATUS_OK) st = safe_read_token(stdin, &sz_tok);
        if (st == STATUS_OK) st = safe_read_token(stdin, &imp_tok);

        if (st != STATUS_OK) {
            free(name);
            free(sz_tok);
            free(imp_tok);
            free_file_descriptors(files, i);
            fprintf(stderr, "Diagnostic: Format error reading file description at index %zu.\n", i + 1);
            return 7;
        }

        size_t f_size = 0;
        size_t f_imp = 0;
        status_t st1 = safe_parse_size_t(sz_tok, &f_size);
        status_t st2 = safe_parse_size_t(imp_tok, &f_imp);
        free(sz_tok);
        free(imp_tok);

        if (st1 != STATUS_OK || st2 != STATUS_OK || f_size == 0) {
            free(name);
            free_file_descriptors(files, i);
            fprintf(stderr, "Diagnostic: Size must be > 0 and importance >= 0 for file at index %zu.\n", i + 1);
            return 8;
        }

        for (size_t prev = 0; prev < i; ++prev) {
            if (safe_str_equal(files[prev].name, name)) {
                free(name);
                free_file_descriptors(files, i);
                fprintf(stderr, "Diagnostic: Duplicate filename detected.\n");
                return 9;
            }
        }

        files[i].name = name;
        files[i].size = f_size;
        files[i].importance = f_imp;
    }

    backup_plan_t *plan = NULL;
    st = select_files_for_backup(files, n, capacity, &plan);
    free_file_descriptors(files, n);

    switch (st) {
        case STATUS_OK:
            printf("TOTAL_SIZE %zu\n", plan->total_size);
            printf("TOTAL_IMPORTANCE %zu\n", plan->total_importance);
            printf("FILES %zu\n", plan->count);
            for (size_t i = 0; i < plan->count; ++i) {
                printf("%s\n", plan->selected_files[i]);
            }
            free_backup_plan(plan);
            free(plan);
            break;
        case STATUS_ERR_MEM:
            fprintf(stderr, "Diagnostic: Memory allocation failed during DP execution.\n");
            return 10;
        case STATUS_ERR_OVERFLOW:
            fprintf(stderr, "Diagnostic: Arithmetic overflow during DP table creation.\n");
            return 11;
        case STATUS_ERR_ARGS:
        default:
            fprintf(stderr, "Diagnostic: Unexpected execution error.\n");
            return 12;
    }

    return 0;
}

