/* opcodes.h
 * 
 * Operation definitions for ConstantRecognition
 * Based on CALC4 from Mathematica SymbolicRegression_ConstantRecognition_codegen.nb
 * 
 * Naming: uppercase Wolfram function names
 * Exception: LOG for Log[x], LOGARITHM reserved for Log[base,x]
 */

#ifndef OPCODES_H
#define OPCODES_H

#define _USE_MATH_DEFINES
#include <math.h>

/* ============================================================================
 * CONSTANTS (push value onto stack)
 * ============================================================================ */

enum {
    /* Core math: pi, e, -1, phi */
    PI, EULER, NEG, GOLDENRATIO,
    /* Digits */
    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
    CONST_COUNT  /* Count */
};

static const double CONST_VALUES[] = {
    M_PI,  
    //3.1415926535897932384626433832795,   /* PI */
    M_E,   
    //2.7182818284590452353602874713527,   /* EULER */
   -1.0,                                      /* NEG */
    1.61803398874989484820458683436563812,   /* GOLDENRATIO */
    1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0
};

static const char* CONST_NAMES[] = {
    "PI", "EULER", "NEG", "GOLDENRATIO",
    "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE"
};

/* ============================================================================
 * UNARY FUNCTIONS (pop one, push one)
 * ============================================================================ */

enum {
    /* Core */
    LOG, EXP,
    /* Elementary functions */
    INV, GAMMA, SQRT, SQR,
    SIN, ARCSIN, COS, ARCCOS, TAN, ARCTAN,
    SINH, ARCSINH, COSH, ARCCOSH, TANH, ARCTANH,
    UNARY_COUNT  /* Count */
};

static const char* UNARY_NAMES[] = {
    "LOG", "EXP",
    "INV", "GAMMA", "SQRT", "SQR",
    "SIN", "ARCSIN", "COS", "ARCCOS", "TAN", "ARCTAN",
    "SINH", "ARCSINH", "COSH", "ARCCOSH", "TANH", "ARCTANH"
};

static inline double apply_unary(int op, double x) {
    switch(op) {
        case LOG:      return log(x);
        case EXP:      return exp(x);
        case INV:      return 1.0 / x;
        case GAMMA:    return tgamma(x);
        case SQRT:     return sqrt(x);
        case SQR:      return x * x;
        case SIN:      return sin(x);
        case ARCSIN:   return asin(x);
        case COS:      return cos(x);
        case ARCCOS:   return acos(x);
        case TAN:      return tan(x);
        case ARCTAN:   return atan(x);
        case SINH:     return sinh(x);
        case ARCSINH:  return asinh(x);
        case COSH:     return cosh(x);
        case ARCCOSH:  return acosh(x);
        case TANH:     return tanh(x);
        case ARCTANH:  return atanh(x);
        default:       return nan("");
    }
}

/* ============================================================================
 * BINARY OPERATORS (pop two, push one)
 * ============================================================================ */

enum {
    /* Commutative */
    PLUS, TIMES,
    /* Non-commutative */
    SUBTRACT, DIVIDE, POWER,   
    BINARY_COUNT  /* Count */
};

static const char* BINARY_NAMES[] = {
    "PLUS", "TIMES", "SUBTRACT", "DIVIDE", "POWER"
};

static inline double apply_binary(int op, double a, double b) {
    switch(op) {
        case PLUS:     return a + b;
        case TIMES:    return a * b;
        case SUBTRACT: return a - b;
        case DIVIDE:   return a / b;
        case POWER:    return pow(a, b);
        default:       return nan("");
    }
}

/* ============================================================================
 * INSTRUCTION SET (runtime configuration)
 * ============================================================================ */

#define MAX_OPS 64

typedef struct {
    int n_const;
    int n_unary;
    int n_binary;
    
    unsigned char const_ops[MAX_OPS];   /* slot -> constant ID */
    unsigned char unary_ops[MAX_OPS];   /* slot -> unary ID */
    unsigned char binary_ops[MAX_OPS];  /* slot -> binary ID */
} InstructionSet;

/* Default: all CALC4 operations, in enum order */
static const InstructionSet CALC4 = {
    .n_const  = CONST_COUNT,   /* 13 */
    .n_unary  = UNARY_COUNT,   /* 18 */
    .n_binary = BINARY_COUNT,  /*  5 */
    
    .const_ops  = { PI, EULER, NEG, GOLDENRATIO,
                    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE },
    .unary_ops  = { LOG, EXP, INV, GAMMA, SQRT, SQR,
                    SIN, ARCSIN, COS, ARCCOS, TAN, ARCTAN,
                    SINH, ARCSINH, COSH, ARCCOSH, TANH, ARCTANH },
    .binary_ops = { PLUS, TIMES, SUBTRACT, DIVIDE, POWER }
};

#endif /* OPCODES_H */