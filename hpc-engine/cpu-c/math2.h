/* math2.h - Extended math functions (header-only)
 *
 * Author: Andrzej Odrzywolek
 * Date: January 2, 2025
 *
 * All functions are static inline for maximum performance.
 * No separate .c file needed.
 *
 * These are used by calculator definitions (CALC4.h, etc.)
 * to provide named function pointers for binary operations
 * and additional unary operations not in standard <math.h>.
 */

#ifndef MATH2_H
#define MATH2_H

#include <math.h>
#include <complex.h>

/* ============================================================================
 * BINARY OPERATION HELPERS
 * 
 * Standard math.h doesn't provide function pointers for basic operations.
 * These wrappers allow using +, -, *, / as function pointers.
 * ============================================================================ */

static inline double plus(double a, double b)                    { return a + b; }
static inline double times(double a, double b)                   { return a * b; }
static inline double subtract(double a, double b)                { return a - b; }
static inline double divide(double a, double b)                  { return a / b; }
static inline double concat(double a, double b)                  { return a*pow(10.0, 1.0+floor(log10(b))) + b; } // concat[a_, b_] := a*10^(1 + Floor[Log10[b]]) + b
												                 
/* Float versions */                                             
static inline float plusf(float a, float b)                      { return a + b; }
static inline float timesf(float a, float b)                     { return a * b; }
static inline float subtractf(float a, float b)                  { return a - b; }
static inline float dividef(float a, float b)                    { return a / b; }
static inline float concatf(float a, float b)                    { return a*powf(10.0f, 1.0f+floorf(log10f(b))) + b; }

/* Long double versions */
static inline long double plusl(long double a, long double b)    { return a + b; }
static inline long double timesl(long double a, long double b)   { return a * b; }
static inline long double subtractl(long double a, long double b) { return a - b; }
static inline long double dividel(long double a, long double b)  { return a / b; }
static inline long double concatl(long double a, long double b)  { return a*powl(10.0l, 1.0l+floorl(log10l(b))) + b; }

/* ============================================================================
 * UNARY FUNCTIONS - REAL
 * ============================================================================ */

/* Percent operator % of desk calcs */
static inline float       percentf(float x)       { return 0.01f*x; }
static inline double      percent(double x)       { return 0.01*x; }
static inline long double percentl(long double x) { return 0.01l*x; }


/* sqr: x squared, x*x */
static inline float       sqrf(float x)       { return x * x; }
static inline double      sqr(double x)       { return x * x; }
static inline long double sqrl(long double x) { return x * x; }

/* dbl: double, 2*x or x+x */
static inline float       dblf(float x)       { return x + x; }
static inline double      dbl(double x)       { return x + x; }
static inline long double dbll(long double x) { return x + x; }

/* half: x/2 or 0.5*x */
static inline float       halff(float x)       { return 0.5f * x; }
static inline double      half(double x)       { return 0.5 * x; }
static inline long double halfl(long double x) { return 0.5L * x; }

/* suc: successor, x+1 */
static inline float       sucf(float x)       { return x + 1.0f; }
static inline double      suc(double x)       { return x + 1.0; }
static inline long double sucl(long double x) { return x + 1.0L; }

/* pre: predecessor, x-1 */
static inline float       pref(float x)       { return x - 1.0f; }
static inline double      pre(double x)       { return x - 1.0; }
static inline long double prel(long double x) { return x - 1.0L; }

/* inv: reciprocal, 1/x */
static inline float       invf(float x)       { return 1.0f / x; }
static inline double      inv(double x)       { return 1.0 / x; }
static inline long double invl(long double x) { return 1.0L / x; }

/* minus: negation, -x */
static inline float       minusf(float x)       { return -x; }
static inline double      minus(double x)       { return -x; }
static inline long double minusl(long double x) { return -x; }

/* tet: tetration, x^x */
static inline float       tetf(float x)       { return powf(x, x); }
static inline double      tet(double x)       { return pow(x, x); }
static inline long double tetl(long double x) { return powl(x, x); }

/* ln: arbitrary base logarithm, log_x(y) */
static inline float       lnf(float x, float y)             { return logf(y) / logf(x); }
static inline double      ln(double x, double y)            { return log(y) / log(x); }
static inline long double lnl(long double x, long double y) { return logl(y) / logl(x); }

/* eml: exp(x) - log(y) */
static inline float       emlf(float x, float y)             { return expf(x) - logf(y); }
static inline double      eml(double x, double y)            { return exp(x) - log(y); }
static inline long double emll(long double x, long double y) { return expl(x) - logl(y); }

/* ============================================================================
 * UNARY FUNCTIONS - COMPLEX
 * ============================================================================ */

#ifndef _MSC_VER

static inline complex float       csqrf(complex float x)       { return x * x; }
static inline complex double      csqr(complex double x)       { return x * x; }
static inline complex long double csqrl(complex long double x) { return x * x; }

static inline complex float       cdblf(complex float x)       { return x + x; }
static inline complex double      cdbl(complex double x)       { return x + x; }
static inline complex long double cdbll(complex long double x) { return x + x; }

static inline complex float       chalff(complex float x)       { return 0.5f * x; }
static inline complex double      chalf(complex double x)       { return 0.5 * x; }
static inline complex long double chalfl(complex long double x) { return 0.5L * x; }

static inline complex float       csucf(complex float x)       { return x + 1.0f; }
static inline complex double      csuc(complex double x)       { return x + 1.0; }
static inline complex long double csucl(complex long double x) { return x + 1.0L; }

static inline complex float       cpref(complex float x)       { return x - 1.0f; }
static inline complex double      cpre(complex double x)       { return x - 1.0; }
static inline complex long double cprel(complex long double x) { return x - 1.0L; }

static inline complex float       cinvf(complex float x)       { return 1.0f / x; }
static inline complex double      cinv(complex double x)       { return 1.0 / x; }
static inline complex long double cinvl(complex long double x) { return 1.0L / x; }

static inline complex float       ctetf(complex float x)       { return cpowf(x, x); }
static inline complex double      ctet(complex double x)       { return cpow(x, x); }
static inline complex long double ctetl(complex long double x) { return cpowl(x, x); }

static inline complex float       clnf(complex float x, complex float y)             { return clogf(y) / clogf(x); }
static inline complex double      cln(complex double x, complex double y)            { return clog(y) / clog(x); }
static inline complex long double clnl(complex long double x, complex long double y) { return clogl(y) / clogl(x); }

static inline complex float       cemlf(complex float x, complex float y)             { return cexpf(x) - clogf(y); }
static inline complex double      ceml(complex double x, complex double y)            { return cexp(x) - clog(y); }
static inline complex long double cemll(complex long double x, complex long double y) { return cexpl(x) - clogl(y); }

/* Complex Gamma: approximate (real part only) */
static inline complex float       ctgammaf(complex float x)       { return tgammaf(crealf(x)) + 0.0f * I; }
static inline complex double      ctgamma(complex double x)       { return tgamma(creal(x)) + 0.0 * I; }
static inline complex long double ctgammal(complex long double x) { return tgammal(creall(x)) + 0.0L * I; }

#endif

#endif /* MATH2_H */
