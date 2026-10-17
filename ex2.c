#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <stdint.h>
#include <math.h>

typedef enum {
    OK = 0,
    ERR_ARGS,
    ERR_FILE,
    ERR_MEM,
    ERR_DATA,
    ERR_OVERFLOW
} status_t;

typedef struct {
    unsigned int id;
    char *name;
    char *surname;
    double salary;
    double eps;
} Employee;

status_t cmp_dbl(const double a, const double b, const double eps, int * const res) {
    if (!res || eps <= 0.0 || isnan(a) || isnan(b) || isinf(a) || isinf(b)) {
        return ERR_ARGS;
    }
    if (fabs(a - b) < eps) {
        *res = 0;
    } else if (a < b) {
        *res = -1;
    } else {
        *res = 1;
    }
    return OK;
}

static int is_space_char(const char c) {
    return (c == ' ' || c == '\t' || c == '\r');
}

static int check_latin(const char * const s) {
    if (!s || *s == '\0') {
        return 0;
    }
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

static status_t parse_uint(const char * const s, unsigned int * const val) {
    if (!s || !val || *s == '\0') {
        return ERR_DATA;
    }
    if (*s == '0' && *(s + 1) != '\0') {
        return ERR_DATA;
    }
    unsigned int acc = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return ERR_DATA;
        }
        unsigned int d = (unsigned int)(s[i] - '0');
        if (acc > (UINT_MAX - d) / 10U) {
            return ERR_OVERFLOW;
        }
        acc = acc * 10U + d;
    }
    *val = acc;
    return OK;
}

static status_t parse_dbl(const char * const s, double * const val) {
    if (!s || !val || *s == '\0') {
        return ERR_DATA;
    }
    if (*s == '.' || (*s == '0' && *(s + 1) != '\0' && *(s + 1) != '.')) {
        return ERR_DATA;
    }

    double res = 0.0;
    double frac = 0.0;
    double div = 10.0;
    int dot = 0;
    int digits_before = 0;
    int digits_after = 0;

    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == '.') {
            if (dot) {
                return ERR_DATA;
            }
            dot = 1;
        } else if (s[i] >= '0' && s[i] <= '9') {
            int d = s[i] - '0';
            if (!dot) {
                digits_before++;
                if (res > (DBL_MAX - (double)d) / 10.0) {
                    return ERR_OVERFLOW;
                }
                res = res * 10.0 + (double)d;
            } else {
                digits_after++;
                if (!isinf(div)) {
                    frac += (double)d / div;
                    div *= 10.0;
                }
            }
        } else {
            return ERR_DATA;
        }
    }

    if (digits_before == 0 || (dot && digits_after == 0)) {
        return ERR_DATA;
    }
    if (res > DBL_MAX - frac) {
        return ERR_OVERFLOW;
    }

    *val = res + frac;
    return OK;
}
static const char *norm_path(const char *p) {
    while (*p == '.' && (*(p + 1) == '/' || *(p + 1) == '\\')) {
        p += 2;
        while (*p == '/' || *p == '\\') {
            p++;
        }
    }
    return p;
}

static int is_same_path(const char * const p1, const char * const p2) {
    if (!p1 || !p2) {
        return 0;
    }
    const char *s1 = norm_path(p1);
    const char *s2 = norm_path(p2);
    while (*s1 != '\0' && *s2 != '\0') {
        char c1 = (*s1 == '\\') ? '/' : *s1;
        char c2 = (*s2 == '\\') ? '/' : *s2;
        if (c1 != c2) {
            return 0;
        }
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

int sort_asc(const void *a, const void *b) {
    const Employee *e1 = (const Employee *)a;
    const Employee *e2 = (const Employee *)b;

    int c = 0;
    cmp_dbl(e1->salary, e2->salary, e1->eps, &c);
    if (c != 0) {
        return c;
    }

    c = strcmp(e1->surname, e2->surname);
    if (c != 0) {
        return c;
    }

    c = strcmp(e1->name, e2->name);
    if (c != 0) {
        return c;
    }

    if (e1->id < e2->id) return -1;
    if (e1->id > e2->id) return 1;
    return 0;
}

int sort_desc(const void *a, const void *b) {
    return -sort_asc(a, b);
}

void clear_list(Employee * const arr, const size_t n) {
    if (!arr) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        free(arr[i].name);
        free(arr[i].surname);
    }
    free(arr);
}

static status_t read_line(FILE * const fp, char ** const out_line, int * const is_eof) {
    size_t cap = 64;
    size_t len = 0;
    char *buf = (char *)malloc(cap * sizeof(char));
    if (!buf) {
        return ERR_MEM;
    }

    int ch;
    *is_eof = 0;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\0') {
            free(buf);
            return ERR_DATA;
        }
        if (ch == '\n') {
            break;
        }
        if (len + 1 >= cap) {
            if (cap > SIZE_MAX / 2) {
                free(buf);
                return ERR_OVERFLOW;
            }
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap * sizeof(char));
            if (!tmp) {
                free(buf);
                return ERR_MEM;
            }
            buf = tmp;
        }
        buf[len++] = (char)ch;
    }

    if (ferror(fp)) {
        free(buf);
        return ERR_FILE;
    }

    if (ch == EOF && len == 0) {
        free(buf);
        *out_line = NULL;
        *is_eof = 1;
        return OK;
    }

    buf[len] = '\0';
    *out_line = buf;
    return OK;
}
static status_t parse_employee_line(char * const line, const double eps, Employee * const emp, int * const is_empty) {
    char *tok_id = NULL;
    char *tok_name = NULL;
    char *tok_sur = NULL;
    char *tok_sal = NULL;
    int t_cnt = 0;
    char *p = line;

    while (*p != '\0') {
        while (*p != '\0' && is_space_char(*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (t_cnt == 0) tok_id = p;
        else if (t_cnt == 1) tok_name = p;
        else if (t_cnt == 2) tok_sur = p;
        else if (t_cnt == 3) tok_sal = p;
        else return ERR_DATA;

        t_cnt++;
        while (*p != '\0' && !is_space_char(*p)) {
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    if (t_cnt == 0) {
        *is_empty = 1;
        return OK;
    }
    *is_empty = 0;

    if (t_cnt != 4) {
        return ERR_DATA;
    }

    unsigned int id = 0;
    double sal = 0.0;
    status_t st = parse_uint(tok_id, &id);
    if (st != OK) return st;

    st = parse_dbl(tok_sal, &sal);
    if (st != OK) return st;

    if (!check_latin(tok_name) || !check_latin(tok_sur)) {
        return ERR_DATA;
    }

    size_t n_len = strlen(tok_name);
    size_t s_len = strlen(tok_sur);

    emp->name = (char *)malloc((n_len + 1) * sizeof(char));
    emp->surname = (char *)malloc((s_len + 1) * sizeof(char));
    if (!emp->name || !emp->surname) {
        free(emp->name);
        free(emp->surname);
        return ERR_MEM;
    }

    strcpy(emp->name, tok_name);
    strcpy(emp->surname, tok_sur);
    emp->id = id;
    emp->salary = sal;
    emp->eps = eps;

    return OK;
}

status_t load_data(FILE * const fp, const double eps, Employee ** const out, size_t * const cnt) {
    if (!fp || !out || !cnt || eps <= 0.0) {
        return ERR_ARGS;
    }

    size_t cap = 8;
    size_t n = 0;
    Employee *arr = (Employee *)malloc(cap * sizeof(Employee));
    if (!arr) {
        return ERR_MEM;
    }

    for (;;) {
        char *line = NULL;
        int is_eof = 0;
        status_t st = read_line(fp, &line, &is_eof);
        if (st != OK) {
            clear_list(arr, n);
            return st;
        }
        if (is_eof) {
            break;
        }

        Employee tmp_emp = {0, NULL, NULL, 0.0, eps};
        int is_empty = 0;
        st = parse_employee_line(line, eps, &tmp_emp, &is_empty);
        free(line);

        if (st != OK) {
            clear_list(arr, n);
            return st;
        }
        if (is_empty) {
            continue;
        }

        for (size_t i = 0; i < n; i++) {
            if (arr[i].id == tmp_emp.id) {
                free(tmp_emp.name);
                free(tmp_emp.surname);
                clear_list(arr, n);
                return ERR_DATA;
            }
        }

        if (n == cap) {
            if (cap > SIZE_MAX / (2 * sizeof(Employee))) {
                free(tmp_emp.name);
                free(tmp_emp.surname);
                clear_list(arr, n);
                return ERR_OVERFLOW;
            }
            cap *= 2;
            Employee *tmp = (Employee *)realloc(arr, cap * sizeof(Employee));
            if (!tmp) {
                free(tmp_emp.name);
                free(tmp_emp.surname);
                clear_list(arr, n);
                return ERR_MEM;
            }
            arr = tmp;
        }

        arr[n++] = tmp_emp;
    }

    *out = arr;
    *cnt = n;
    return OK;
}

status_t save_data(FILE * const fp, const Employee * const arr, const size_t n) {
    if (!fp || (!arr && n > 0)) {
        return ERR_ARGS;
    }
    for (size_t i = 0; i < n; i++) {
        if (fprintf(fp, "%u %s %s %.2f\n", arr[i].id, arr[i].name, arr[i].surname, arr[i].salary) < 0) {
            return ERR_FILE;
        }
    }
    if (fflush(fp) != 0) {
        return ERR_FILE;
    }
    return OK;
}

int main(int argc, char **argv) {
    if (argc != 4 || !argv) {
        printf("Invalid arguments\n");
        return 1;
    }

    const char *in_p = *(argv + 1);
    const char *flag = *(argv + 2);
    const char *out_p = *(argv + 3);

    if (!in_p || !flag || !out_p || *in_p == '\0' || *out_p == '\0') {
        printf("Invalid arguments\n");
        return 1;
    }

    if (strlen(flag) != 2 || (*flag != '-' && *flag != '/') || (*(flag + 1) != 'a' && *(flag + 1) != 'd')) {
        printf("Invalid flag\n");
        return 1;
    }

    if (is_same_path(in_p, out_p)) {
        printf("Same file error\n");
        return 1;
    }

    FILE *fin = fopen(in_p, "r");
    if (!fin) {
        printf("File open error\n");
        return 1;
    }

    const double eps = 1e-7;
    Employee *list = NULL;
    size_t n = 0;

    status_t st = load_data(fin, eps, &list, &n);
    fclose(fin);

    if (st != OK) {
        printf("Read error %d\n", st);
        return 1;
    }

    if (n > 0) {
        qsort(list, n, sizeof(Employee), (*(flag + 1) == 'a') ? sort_asc : sort_desc);
    }

    FILE *fout = fopen(out_p, "w");
    if (!fout) {
        clear_list(list, n);
        printf("File write error\n");
        return 1;
    }

    st = save_data(fout, list, n);
    if (fclose(fout) != 0 || st != OK) {
        clear_list(list, n);
        printf("Write error\n");
        return 1;
    }

    clear_list(list, n);
    return 0;
}

