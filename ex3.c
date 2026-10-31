#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_MEM,
    STATUS_ERR_FILE,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_FORMAT
} status_t;

typedef struct {
    size_t line;
    size_t character;
} match_pos_t;

typedef struct {
    char *filepath;
    match_pos_t *matches;
    size_t count;
    size_t capacity;
} file_result_t;

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

static status_t read_file_to_buffer(
    const char * const filepath,
    char ** const out_buf,
    size_t * const out_size
) {
    if (filepath == NULL || out_buf == NULL || out_size == NULL) {
        return STATUS_ERR_ARGS;
    }

    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        return STATUS_ERR_FILE;
    }

    size_t capacity = 128;
    size_t size = 0;
    char *buffer = (char *)malloc(capacity * sizeof(char));
    if (buffer == NULL) {
        fclose(fp);
        return STATUS_ERR_MEM;
    }

    int ch = 0;
    while ((ch = fgetc(fp)) != EOF) {
        if (size >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                fclose(fp);
                return STATUS_ERR_OVERFLOW;
            }
            size_t new_cap = capacity * 2;
            char *new_buf = (char *)realloc(buffer, new_cap * sizeof(char));
            if (new_buf == NULL) {
                free(buffer);
                fclose(fp);
                return STATUS_ERR_MEM;
            }
            buffer = new_buf;
            capacity = new_cap;
        }
        buffer[size++] = (char)ch;
    }

    fclose(fp);
    *out_buf = buffer;
    *out_size = size;
    return STATUS_OK;
}

static status_t build_kmp_pi(
    const char * const pattern,
    const size_t pattern_len,
    size_t ** const out_pi
) {
    if (pattern == NULL || pattern_len == 0 || out_pi == NULL) {
        return STATUS_ERR_ARGS;
    }

    size_t *pi = (size_t *)malloc(pattern_len * sizeof(size_t));
    if (pi == NULL) {
        return STATUS_ERR_MEM;
    }

    pi[0] = 0;
    for (size_t i = 1; i < pattern_len; ++i) {
        size_t j = pi[i - 1];
        while (j > 0 && pattern[i] != pattern[j]) {
            j = pi[j - 1];
        }
        if (pattern[i] == pattern[j]) {
            j++;
        }
        pi[i] = j;
    }

    *out_pi = pi;
    return STATUS_OK;
}

static status_t add_match(
    file_result_t * const res,
    const size_t line,
    const size_t character
) {
    if (res == NULL) {
        return STATUS_ERR_ARGS;
    }

    if (res->count >= res->capacity) {
        size_t new_cap = (res->capacity == 0) ? 8 : res->capacity * 2;
        if (new_cap > SIZE_MAX / sizeof(match_pos_t)) {
            return STATUS_ERR_OVERFLOW;
        }
        match_pos_t *new_arr = (match_pos_t *)realloc(
            res->matches,
            new_cap * sizeof(match_pos_t)
        );
        if (new_arr == NULL) {
            return STATUS_ERR_MEM;
        }
        res->matches = new_arr;
        res->capacity = new_cap;
    }

    res->matches[res->count].line = line;
    res->matches[res->count].character = character;
    res->count++;
    return STATUS_OK;
}

static status_t search_pattern_in_text(
    const char * const text,
    const size_t text_len,
    const char * const pattern,
    const size_t pattern_len,
    file_result_t * const res
) {
    if (text == NULL && text_len > 0) return STATUS_ERR_ARGS;
    if (pattern == NULL || pattern_len == 0 || res == NULL) return STATUS_ERR_ARGS;

    size_t *pi = NULL;
    status_t st = build_kmp_pi(pattern, pattern_len, &pi);
    if (st != STATUS_OK) return st;

    size_t j = 0;
    size_t scan_idx = 0;
    size_t cur_line = 1;
    size_t cur_char = 1;

    for (size_t i = 0; i < text_len; ++i) {
        while (j > 0 && text[i] != pattern[j]) {
            j = pi[j - 1];
        }
        if (text[i] == pattern[j]) {
            j++;
        }

        if (j == pattern_len) {
            size_t match_start = i + 1 - pattern_len;

            while (scan_idx < match_start) {
                if (text[scan_idx] == '\n') {
                    cur_line++;
                    cur_char = 1;
                } else {
                    cur_char++;
                }
                scan_idx++;
            }

            st = add_match(res, cur_line, cur_char);
            if (st != STATUS_OK) {
                free(pi);
                return st;
            }

            j = pi[j - 1];
        }
    }

    free(pi);
    return STATUS_OK;
}
static void free_file_results(file_result_t * const results, const size_t count) {
    if (results == NULL) return;
    for (size_t i = 0; i < count; ++i) {
        if (results[i].filepath != NULL) {
            free(results[i].filepath);
        }
        if (results[i].matches != NULL) {
            free(results[i].matches);
        }
    }
    free(results);
}

status_t find_substring_in_files(
    const char * const substring,
    const size_t file_count,
    file_result_t ** const out_results,
    ...
) {
    if (substring == NULL || out_results == NULL || file_count == 0) {
        return STATUS_ERR_ARGS;
    }

    size_t sub_len = 0;
    status_t st = safe_strlen(substring, &sub_len);
    if (st != STATUS_OK) return st;
    if (sub_len == 0) return STATUS_ERR_FORMAT;

    if (file_count > SIZE_MAX / sizeof(file_result_t)) {
        return STATUS_ERR_OVERFLOW;
    }

    file_result_t *results = (file_result_t *)calloc(file_count, sizeof(file_result_t));
    if (results == NULL) {
        return STATUS_ERR_MEM;
    }

    va_list args;
    va_start(args, out_results);

    for (size_t i = 0; i < file_count; ++i) {
        const char *filepath = va_arg(args, const char *);
        if (filepath == NULL) {
            st = STATUS_ERR_ARGS;
            break;
        }

        st = safe_strdup(filepath, &results[i].filepath);
        if (st != STATUS_OK) break;

        char *file_data = NULL;
        size_t file_len = 0;
        st = read_file_to_buffer(filepath, &file_data, &file_len);
        if (st != STATUS_OK) break;

        st = search_pattern_in_text(file_data, file_len, substring, sub_len, &results[i]);
        free(file_data);
        if (st != STATUS_OK) break;
    }

    va_end(args);

    if (st != STATUS_OK) {
        free_file_results(results, file_count);
        return st;
    }

    *out_results = results;
    return STATUS_OK;
}

static void print_search_report(
    const file_result_t * const results,
    const size_t file_count,
    const char * const pattern
) {
    printf("Pattern: \"%s\"\n", pattern);
    for (size_t i = 0; i < file_count; ++i) {
        printf("File: %s\n", results[i].filepath);
        if (results[i].count == 0) {
            printf("  Matches not found.\n");
        } else {
            for (size_t m = 0; m < results[i].count; ++m) {
                printf("  [Line %zu, Character %zu]\n",
                       results[i].matches[m].line,
                       results[i].matches[m].character);
            }
        }
    }
}

int main(void) {
    const char *sample_file_1 = "test1.txt";
    const char *sample_file_2 = "test2.txt";

    FILE *f1 = fopen(sample_file_1, "wb");
    if (f1 != NULL) {
        fputs("hello world\nmultiline\ntest case with world inside", f1);
        fclose(f1);
    }
    FILE *f2 = fopen(sample_file_2, "wb");
    if (f2 != NULL) {
        fputs("other\nworld\nworld", f2);
        fclose(f2);
    }

    const char *target = "world";
    file_result_t *report = NULL;
    size_t files_num = 2;

    status_t st = find_substring_in_files(
        target,
        files_num,
        &report,
        sample_file_1,
        sample_file_2
    );

    switch (st) {
        case STATUS_OK:
            print_search_report(report, files_num, target);
            free_file_results(report, files_num);
            break;
        case STATUS_ERR_ARGS:
            fprintf(stderr, "Diagnostic: Invalid argument passed to find_substring_in_files.\n");
            return 1;
        case STATUS_ERR_FORMAT:
            fprintf(stderr, "Diagnostic: Substring cannot be empty.\n");
            return 2;
        case STATUS_ERR_FILE:
            fprintf(stderr, "Diagnostic: File opening or reading failure.\n");
            return 3;
        case STATUS_ERR_MEM:
            fprintf(stderr, "Diagnostic: Memory allocation failure.\n");
            return 4;
        case STATUS_ERR_OVERFLOW:
            fprintf(stderr, "Diagnostic: Buffer overflow during processing.\n");
            return 5;
        default:
            fprintf(stderr, "Diagnostic: Unknown error.\n");
            return 6;
    }

    remove(sample_file_1);
    remove(sample_file_2);

    return 0;
}
