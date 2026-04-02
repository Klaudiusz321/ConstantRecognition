#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>
#include "math2.h"
#define STACKSIZE 16
#define MAXTHREADS 128
#define True 1
#define False 0
#define INSTR_NUM 36

/*
Unary functions:
                               1                                             2
{{LOG, Log}, {EXP, Exp}, {INV, -- & }, {GAMMA, Gamma}, {SQRT, Sqrt}, {SQR, #1  & }, {SIN, Sin}, {ARCSIN, ArcSin}, {COS, Cos}, {ARCCOS, ArcCos}, {TAN, Tan}, {ARCTAN, ArcTan}, {SINH, Sinh}, {ARCSINH, ArcSinh}, {COSH, Cosh}, {ARCCOSH, ArcCosh}, {TANH, Tanh}, {ARCTANH, ArcTanh}}
                               #1
Push on stack:
{{PI, Pi}, {EULER, E}, {NEG, -1}, {GOLDENRATIO, GoldenRatio}, {ONE, 1}, {TWO, 2}, {THREE, 3}, {FOUR, 4}, {FIVE, 5}, {SIX, 6}, {SEVEN, 7}, {EIGHT, 8}, {NINE, 9}}
Binary operators:
{{PLUS, Plus}, {TIMES, Times}, {SUBTRACT, Subtract}, {DIVIDE, Divide}, {POWER, Power}}

{0 -> PI, 1 -> EULER, 2 -> NEG, 3 -> GOLDENRATIO, 4 -> LOG, 5 -> EXP, 6 -> PLUS, 7 -> TIMES, 8 -> INV, 9 -> GAMMA, a -> SQRT, b -> SQR, c -> SIN, d -> ARCSIN, e -> COS, f -> ARCCOS, g -> TAN, h -> ARCTAN, i -> SINH, j -> ARCSINH, k -> COSH, l -> ARCCOSH, m -> TANH, n -> ARCTANH, o -> ONE, p -> TWO, q -> THREE, r -> FOUR, s -> FIVE, t -> SIX, u -> SEVEN, v -> EIGHT, w -> NINE, x -> SUBTRACT, y -> DIVIDE, z -> POWER}

*/
float constantf(const char * , const int);
double constant(const char * , const int);
long double constantl(const char * , const int);
float complex cconstantf(const char * , const int);
double complex cconstant(const char * , const int);
long double complex cconstantl(const char * , const int);

int checkSyntax(const char * , const int);
