/* utils.c - Utility functions and error norms
 *
 * Author: Andrzej Odrzywolek
 * Date: January 2, 2025
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

/* ============================================================================
 * PORTABLE POPCOUNT
 * ============================================================================ */

#ifdef _MSC_VER
#include <intrin.h>
static inline int popcount64(uint64_t x) {
    return (int)__popcnt64(x);
}
#elif defined(__GNUC__) || defined(__clang__)
static inline int popcount64(uint64_t x) {
    return __builtin_popcountll(x);
}
#else
/* Fallback software implementation */
static inline int popcount64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
}
#endif

/* ============================================================================
 * ULP DISTANCE
 * 
 * Based on the insight that IEEE 754 doubles, when reinterpreted as
 * signed integers with appropriate sign handling, preserve numerical
 * ordering. The difference between two such integers gives the ULP distance.
 * ============================================================================ */

uint64_t compute_ULP_distance(double ref, double val)
{
    /* Handle NaN and infinities */
    if (isnan(ref) || isnan(val) || isinf(ref) || isinf(val)) {
        return UINT64_MAX;
    }

    /* Map doubles to int64_t while preserving ordering */
    int64_t i_ref, i_val;
    memcpy(&i_ref, &ref, sizeof(double));
    memcpy(&i_val, &val, sizeof(double));

    /* Handle negative numbers: flip to maintain ordering */
    if (i_ref < 0)
        i_ref = INT64_MIN - i_ref;
    if (i_val < 0)
        i_val = INT64_MIN - i_val;

    /* Compute absolute difference */
    uint64_t ulp_diff = (uint64_t)(llabs(i_val - i_ref));
    return ulp_diff;
}

/* ============================================================================
 * HAMMING DISTANCE
 * 
 * Count differing bits between IEEE 754 representations.
 * ============================================================================ */

int compute_hamming_distance(double a, double b)
{
    uint64_t ua, ub;
    memcpy(&ua, &a, sizeof(double));
    memcpy(&ub, &b, sizeof(double));
    return popcount64(ua ^ ub);
}

/* ============================================================================
 * COMPLEX NUMBER PARSING
 * ============================================================================ */
#ifndef _MSC_VER
#include <complex.h>

double complex parseComplex(const char* str)
{
    double realPart = 0.0, imagPart = 0.0;
    double complex result;
    char sign = '+';
    int count;

    /* Try reading as a complex number: "real +/- imag i" */
    count = sscanf(str, "%lf %c %lfi", &realPart, &sign, &imagPart);
    
    if (count == 3) {
        if (sign == '-') {
            imagPart = -imagPart;
        }
        result = realPart + imagPart * I;
    }
    /* Try reading as a purely real number */
    else if (sscanf(str, "%lf", &realPart) == 1) {
        result = realPart + 0.0 * I;
    } else {
        /* Return NAN + NAN * I to indicate error */
        result = NAN + NAN * I;
    }

    return result;
}

#endif