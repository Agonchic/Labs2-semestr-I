#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#define MAX_CLI_FRACTIONS 16

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_MEM
} status_t;

static long long gcd_ll(long long a, long long b) {
    while (b != 0) {
        long long rem = a % b;
        a = b;
        b = rem;
    }
    return a;
}

static int double_to_fraction(const double val, const double eps, long long * const num, long long * const den) {
    if (!num || !den || val <= 0.0 || val >= 1.0 || eps <= 0.0 || isnan(val) || isinf(val)) {
        return -1;
    }

    long long p_prev2 = 0, p_prev1 = 1;
    long long q_prev2 = 1, q_prev1 = 0;
    double x = val;

    for (int iter = 0; iter < 64; ++iter) {
        long long a = (long long)floor(x);
        long long p_curr = a * p_prev1 + p_prev2;
        long long q_curr = a * q_prev1 + q_prev2;

        if (q_curr > 0) {
            double approx = (double)p_curr / (double)q_curr;
            if (fabs(approx - val) <= eps) {
                long long g = gcd_ll(p_curr, q_curr);
                *num = p_curr / g;
                *den = q_curr / g;
                return 0;
            }
        }

        p_prev2 = p_prev1;
        p_prev1 = p_curr;
        q_prev2 = q_prev1;
        q_prev1 = q_curr;

        double rem = x - (double)a;
        if (fabs(rem) < 1e-15) {
            long long g = gcd_ll(p_prev1, q_prev1);
            *num = p_prev1 / g;
            *den = q_prev1 / g;
            return 0;
        }
        x = 1.0 / rem;
    }

    long long g = gcd_ll(p_prev1, q_prev1);
    *num = p_prev1 / g;
    *den = q_prev1 / g;
    return 0;
}

static int has_finite_representation(const double val, const int base, const double eps) {
    long long num = 0, den = 0;
    if (double_to_fraction(val, eps, &num, &den) != 0) {
        return -1;
    }

    long long q = den;
    long long b = (long long)base;
    long long g;

    while ((g = gcd_ll(q, b)) > 1) {
        while (q % g == 0) {
            q /= g;
        }
    }

    return (q == 1) ? 1 : 0;
}

status_t check_fractions_finite(
    const double eps,
    const int base,
    const int count,
    int ** const out,
    ...
) {
    if (base < 2 || base > 36 || count <= 0 || eps <= 0.0 || isnan(eps) || isinf(eps) || !out) {
        return STATUS_ERR_ARGS;
    }

    int *results = (int *)malloc(sizeof(int) * (size_t)count);
    if (!results) {
        return STATUS_ERR_MEM;
    }

    va_list ap;
    va_start(ap, out);

    for (int i = 0; i < count; ++i) {
        double val = va_arg(ap, double);
        if (isnan(val) || isinf(val) || val <= eps || (1.0 - val) <= eps) {
            va_end(ap);
            free(results);
            return STATUS_ERR_ARGS;
        }

        int check_res = has_finite_representation(val, base, eps);
        if (check_res == -1) {
            va_end(ap);
            free(results);
            return STATUS_ERR_ARGS;
        }
        results[i] = check_res;
    }

    va_end(ap);
    *out = results;
    return STATUS_OK;
}

static status_t dispatch_variadic_check(
    const double eps,
    const int base,
    const int count,
    int ** const out,
    const double * const arr
) {
    if (!arr || !out) return STATUS_ERR_ARGS;

    switch (count) {
        case 1:  return check_fractions_finite(eps, base, 1, out, arr[0]);
        case 2:  return check_fractions_finite(eps, base, 2, out, arr[0], arr[1]);
        case 3:  return check_fractions_finite(eps, base, 3, out, arr[0], arr[1], arr[2]);
        case 4:  return check_fractions_finite(eps, base, 4, out, arr[0], arr[1], arr[2], arr[3]);
        case 5:  return check_fractions_finite(eps, base, 5, out, arr[0], arr[1], arr[2], arr[3], arr[4]);
        case 6:  return check_fractions_finite(eps, base, 6, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
        case 7:  return check_fractions_finite(eps, base, 7, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6]);
        case 8:  return check_fractions_finite(eps, base, 8, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7]);
        case 9:  return check_fractions_finite(eps, base, 9, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8]);
        case 10: return check_fractions_finite(eps, base, 10, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9]);
        case 11: return check_fractions_finite(eps, base, 11, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10]);
        case 12: return check_fractions_finite(eps, base, 12, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11]);
        case 13: return check_fractions_finite(eps, base, 13, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12]);
        case 14: return check_fractions_finite(eps, base, 14, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13]);
        case 15: return check_fractions_finite(eps, base, 15, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14]);
        case 16: return check_fractions_finite(eps, base, 16, out, arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14], arr[15]);
        default: return STATUS_ERR_ARGS;
    }
}

int main(int argc, char *argv[]) {
    const double eps = 1e-9;

    if (argc < 3) {
        fprintf(stderr, "Error: at least base and one fraction required.\n");
        return 1;
    }

    const int count = argc - 2;
    if (count > MAX_CLI_FRACTIONS) {
        fprintf(stderr, "Error: maximum %d fractions supported via CLI.\n", MAX_CLI_FRACTIONS);
        return 1;
    }

    char *endptr = NULL;
    long base_long = strtol(argv[1], &endptr, 10);
    if (!endptr || *endptr != '\0' || base_long < 2 || base_long > 36) {
        fprintf(stderr, "Error: invalid base '%s'. Must be an integer in range [2..36].\n", argv[1]);
        return 1;
    }
    const int base = (int)base_long;

    double *arr = (double *)malloc(sizeof(double) * (size_t)count);
    if (!arr) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 1;
    }

    for (int i = 0; i < count; ++i) {
        endptr = NULL;
        arr[i] = strtod(argv[i + 2], &endptr);
        if (!endptr || *endptr != '\0' || isnan(arr[i]) || isinf(arr[i]) || arr[i] <= eps || (1.0 - arr[i]) <= eps) {
            fprintf(stderr, "Error: invalid fraction argument '%s'. Must be a real number in range (0; 1).\n", argv[i + 2]);
            free(arr);
            return 1;
        }
    }

    int *results = NULL;
    status_t st = dispatch_variadic_check(eps, base, count, &results, arr);
    if (st != STATUS_OK || !results) {
        fprintf(stderr, "Error: check failed with status code %d.\n", st);
        free(arr);
        free(results);
        return 1;
    }

    for (int i = 0; i < count; ++i) {
        printf("%g: %s\n", arr[i], results[i] == 1 ? "FINITE" : "INFINITE");
    }

    free(arr);
    free(results);
    return 0;
}






