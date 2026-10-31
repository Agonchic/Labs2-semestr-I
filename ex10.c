#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <float.h>

typedef enum {
    OK = 0,
    ERR_ARGS,
    ERR_MEM,
    ERR_OVERFLOW
} status_t;

static double d_abs(const double x) {
    return (x < 0.0) ? -x : x;
}

static int check_dbl(const double x) {
    if (x != x) {
        return 0;
    }
    if (x > DBL_MAX || x < -DBL_MAX) {
        return 0;
    }
    return 1;
}

status_t is_zero(const double val, const double eps, int * const res) {
    if (!res || eps <= 0.0 || !check_dbl(val) || !check_dbl(eps)) {
        return ERR_ARGS;
    }
    *res = (d_abs(val) < eps) ? 1 : 0;
    return OK;
}
status_t calc_poly(const double eps, const double a, double ** const out,
                   const int deg, ...) {
    if (!out || deg < 0 || eps <= 0.0 || !check_dbl(eps) || !check_dbl(a)) {
        return ERR_ARGS;
    }

    size_t n = (size_t)deg;
    if (n >= SIZE_MAX / sizeof(double)) {
        return ERR_OVERFLOW;
    }

    double *g = (double *)malloc((n + 1) * sizeof(double));
    if (!g) {
        return ERR_MEM;
    }

    va_list ap;
    va_start(ap, deg);
    for (size_t i = 0; i <= n; i++) {
        double c = va_arg(ap, double);
        if (!check_dbl(c)) {
            va_end(ap);
            free(g);
            return ERR_ARGS;
        }
        g[i] = c;
    }
    va_end(ap);

    for (size_t k = 0; k <= n; k++) {
        for (size_t i = n; i > k; i--) {
            double prod = g[i] * a;
            if (!check_dbl(prod)) {
                free(g);
                return ERR_OVERFLOW;
            }
            double sum = g[i - 1] + prod;
            if (!check_dbl(sum)) {
                free(g);
                return ERR_OVERFLOW;
            }
            g[i - 1] = sum;
        }
    }

    for (size_t i = 0; i <= n; i++) {
        int z = 0;
        status_t st = is_zero(g[i], eps, &z);
        if (st != OK) {
            free(g);
            return st;
        }
        if (z) {
            g[i] = 0.0;
        }
    }

    *out = g;
    return OK;
}
int main(void) {
    const double eps = 1e-9;
    const double a = 1.0;
    const int deg = 3;
    double *res = NULL;

    status_t st = calc_poly(eps, a, &res, deg, -2.0, 1.0, -2.0, 1.0);
    if (st != OK) {
        printf("Error %d\n", st);
        return 1;
    }

    for (int i = 0; i <= deg; i++) {
        printf("g_%d = %.6f\n", i, res[i]);
    }

    free(res);
    return 0;
}
