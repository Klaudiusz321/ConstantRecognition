/* utils.h - Utility functions and error norms
 *
 * Author: Andrzej Odrzywolek
 * Date: January 2, 2025
 *
 * Error norms:
 *   - ULP (Units in Last Place) distance
 *   - Hamming distance on IEEE 754 representation
 *
 * Utility functions:
 *   - Complex number parsing
 */

#ifndef UTILS_H
#define UTILS_H

/* Windows compatibility */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _WIN32
#define strdup _strdup
#define sscanf sscanf_s
#endif

#include <stdint.h>
#include <complex.h>

/* ============================================================================
 * ULP DISTANCE
 * 
 * Computes the number of representable floating-point values between
 * two doubles. Returns UINT64_MAX for NaN/Inf inputs.
 *
 * This is a discrete metric useful for exact matching within
 * floating-point precision limits.
 * ============================================================================ */

uint64_t compute_ULP_distance(double ref, double val);

/* ============================================================================
 * HAMMING DISTANCE
 * 
 * Computes the number of differing bits in the IEEE 754 representation
 * of two doubles. Returns a value from 0 to 64.
 *
 * This metric is sensitive to all bits, not just the mantissa,
 * so it can detect sign and exponent differences.
 * ============================================================================ */

int compute_hamming_distance(double a, double b);

/* ============================================================================
 * COMPLEX NUMBER PARSING
 * 
 * Parse complex number from string format: "real + imag*i" or just "real"
 * Returns NAN + NAN*I on parse error.
 * ============================================================================ */

#ifndef _MSC_VER

double complex parseComplex(const char* str);

#endif

#endif /* UTILS_H */
