#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>
#include "math2.h"

#include "constant.h"

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

int checkSyntax(const char * amino, const int aminoLength)
{
  int ii,position;
  char instr;

  position = 0;
  for(ii=0;ii<aminoLength;ii++)
   {
 
	instr = amino[ii];
    switch(instr)
	 {case '0': case '1': case '2': case '3': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': 
	position++;
break;
case '4': case '5': case '8': case '9': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
	if(position<1) return False;
break;
case '6': case '7': case 'x': case 'y': case 'z': 
	if(position<2) return False;
	position--;
break;
}
}
  if(position==1)
   	return True;
  else
   	return False;
 
}

float complex  cconstantf(const char * amino, const int aminoLength)
{
float complex stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571f;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407f;
	position++;
break;
case '2':
	stos[position]=-1.0f;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189f;
	position++;
break;
case 'o':
	stos[position]=1.0f;
	position++;
break;
case 'p':
	stos[position]=2.0f;
	position++;
break;
case 'q':
	stos[position]=3.0f;
	position++;
break;
case 'r':
	stos[position]=4.0f;
	position++;
break;
case 's':
	stos[position]=5.0f;
	position++;
break;
case 't':
	stos[position]=6.0f;
	position++;
break;
case 'u':
	stos[position]=7.0f;
	position++;
break;
case 'v':
	stos[position]=8.0f;
	position++;
break;
case 'w':
	stos[position]=9.0f;
	position++;
break;
case '4':
	stos[position-1]=clogf(stos[position-1]);
break;
case '5':
	stos[position-1]=cexpf(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0f/(stos[position-1]);
break;
case '9':
	stos[position-1]=ctgammaf(stos[position-1]);
break;
case 'a':
	stos[position-1]=csqrtf(stos[position-1]);
break;
case 'b':
	stos[position-1]=csqrf(stos[position-1]);
break;
case 'c':
	stos[position-1]=csinf(stos[position-1]);
break;
case 'd':
	stos[position-1]=casinf(stos[position-1]);
break;
case 'e':
	stos[position-1]=ccosf(stos[position-1]);
break;
case 'f':
	stos[position-1]=cacosf(stos[position-1]);
break;
case 'g':
	stos[position-1]=ctanf(stos[position-1]);
break;
case 'h':
	stos[position-1]=catanf(stos[position-1]);
break;
case 'i':
	stos[position-1]=csinhf(stos[position-1]);
break;
case 'j':
	stos[position-1]=casinhf(stos[position-1]);
break;
case 'k':
	stos[position-1]=ccoshf(stos[position-1]);
break;
case 'l':
	stos[position-1]=cacoshf(stos[position-1]);
break;
case 'm':
	stos[position-1]=ctanhf(stos[position-1]);
break;
case 'n':
	stos[position-1]=catanhf(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=cpowf(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nanf("NAN"));
}
}
return (stos[position-1]);
}
double complex  cconstant(const char * amino, const int aminoLength)
{
double complex stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407;
	position++;
break;
case '2':
	stos[position]=-1.0;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189;
	position++;
break;
case 'o':
	stos[position]=1.0;
	position++;
break;
case 'p':
	stos[position]=2.0;
	position++;
break;
case 'q':
	stos[position]=3.0;
	position++;
break;
case 'r':
	stos[position]=4.0;
	position++;
break;
case 's':
	stos[position]=5.0;
	position++;
break;
case 't':
	stos[position]=6.0;
	position++;
break;
case 'u':
	stos[position]=7.0;
	position++;
break;
case 'v':
	stos[position]=8.0;
	position++;
break;
case 'w':
	stos[position]=9.0;
	position++;
break;
case '4':
	stos[position-1]=clog(stos[position-1]);
break;
case '5':
	stos[position-1]=cexp(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0/(stos[position-1]);
break;
case '9':
	stos[position-1]=ctgamma(stos[position-1]);
break;
case 'a':
	stos[position-1]=csqrt(stos[position-1]);
break;
case 'b':
	stos[position-1]=csqr(stos[position-1]);
break;
case 'c':
	stos[position-1]=csin(stos[position-1]);
break;
case 'd':
	stos[position-1]=casin(stos[position-1]);
break;
case 'e':
	stos[position-1]=ccos(stos[position-1]);
break;
case 'f':
	stos[position-1]=cacos(stos[position-1]);
break;
case 'g':
	stos[position-1]=ctan(stos[position-1]);
break;
case 'h':
	stos[position-1]=catan(stos[position-1]);
break;
case 'i':
	stos[position-1]=csinh(stos[position-1]);
break;
case 'j':
	stos[position-1]=casinh(stos[position-1]);
break;
case 'k':
	stos[position-1]=ccosh(stos[position-1]);
break;
case 'l':
	stos[position-1]=cacosh(stos[position-1]);
break;
case 'm':
	stos[position-1]=ctanh(stos[position-1]);
break;
case 'n':
	stos[position-1]=catanh(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=cpow(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nan("NAN"));
}
}
return (stos[position-1]);
}
long double complex  cconstantl(const char * amino, const int aminoLength)
{
long double complex stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571l;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407l;
	position++;
break;
case '2':
	stos[position]=-1.0l;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189l;
	position++;
break;
case 'o':
	stos[position]=1.0l;
	position++;
break;
case 'p':
	stos[position]=2.0l;
	position++;
break;
case 'q':
	stos[position]=3.0l;
	position++;
break;
case 'r':
	stos[position]=4.0l;
	position++;
break;
case 's':
	stos[position]=5.0l;
	position++;
break;
case 't':
	stos[position]=6.0l;
	position++;
break;
case 'u':
	stos[position]=7.0l;
	position++;
break;
case 'v':
	stos[position]=8.0l;
	position++;
break;
case 'w':
	stos[position]=9.0l;
	position++;
break;
case '4':
	stos[position-1]=clogl(stos[position-1]);
break;
case '5':
	stos[position-1]=cexpl(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0l/(stos[position-1]);
break;
case '9':
	stos[position-1]=ctgammal(stos[position-1]);
break;
case 'a':
	stos[position-1]=csqrtl(stos[position-1]);
break;
case 'b':
	stos[position-1]=csqrl(stos[position-1]);
break;
case 'c':
	stos[position-1]=csinl(stos[position-1]);
break;
case 'd':
	stos[position-1]=casinl(stos[position-1]);
break;
case 'e':
	stos[position-1]=ccosl(stos[position-1]);
break;
case 'f':
	stos[position-1]=cacosl(stos[position-1]);
break;
case 'g':
	stos[position-1]=ctanl(stos[position-1]);
break;
case 'h':
	stos[position-1]=catanl(stos[position-1]);
break;
case 'i':
	stos[position-1]=csinhl(stos[position-1]);
break;
case 'j':
	stos[position-1]=casinhl(stos[position-1]);
break;
case 'k':
	stos[position-1]=ccoshl(stos[position-1]);
break;
case 'l':
	stos[position-1]=cacoshl(stos[position-1]);
break;
case 'm':
	stos[position-1]=ctanhl(stos[position-1]);
break;
case 'n':
	stos[position-1]=catanhl(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=cpowl(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nanl("NAN"));
}
}
return (stos[position-1]);
}
float constantf(const char * amino, const int aminoLength)
{
float stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571f;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407f;
	position++;
break;
case '2':
	stos[position]=-1.0f;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189f;
	position++;
break;
case 'o':
	stos[position]=1.0f;
	position++;
break;
case 'p':
	stos[position]=2.0f;
	position++;
break;
case 'q':
	stos[position]=3.0f;
	position++;
break;
case 'r':
	stos[position]=4.0f;
	position++;
break;
case 's':
	stos[position]=5.0f;
	position++;
break;
case 't':
	stos[position]=6.0f;
	position++;
break;
case 'u':
	stos[position]=7.0f;
	position++;
break;
case 'v':
	stos[position]=8.0f;
	position++;
break;
case 'w':
	stos[position]=9.0f;
	position++;
break;
case '4':
	stos[position-1]=logf(stos[position-1]);
break;
case '5':
	stos[position-1]=expf(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0f/(stos[position-1]);
break;
case '9':
	stos[position-1]=tgammaf(stos[position-1]);
break;
case 'a':
	stos[position-1]=sqrtf(stos[position-1]);
break;
case 'b':
	stos[position-1]=sqrf(stos[position-1]);
break;
case 'c':
	stos[position-1]=sinf(stos[position-1]);
break;
case 'd':
	stos[position-1]=asinf(stos[position-1]);
break;
case 'e':
	stos[position-1]=cosf(stos[position-1]);
break;
case 'f':
	stos[position-1]=acosf(stos[position-1]);
break;
case 'g':
	stos[position-1]=tanf(stos[position-1]);
break;
case 'h':
	stos[position-1]=atanf(stos[position-1]);
break;
case 'i':
	stos[position-1]=sinhf(stos[position-1]);
break;
case 'j':
	stos[position-1]=asinhf(stos[position-1]);
break;
case 'k':
	stos[position-1]=coshf(stos[position-1]);
break;
case 'l':
	stos[position-1]=acoshf(stos[position-1]);
break;
case 'm':
	stos[position-1]=tanhf(stos[position-1]);
break;
case 'n':
	stos[position-1]=atanhf(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=powf(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nanf("NAN"));
}
}
return (stos[position-1]);
}
double constant(const char * amino, const int aminoLength)
{
double stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407;
	position++;
break;
case '2':
	stos[position]=-1.0;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189;
	position++;
break;
case 'o':
	stos[position]=1.0;
	position++;
break;
case 'p':
	stos[position]=2.0;
	position++;
break;
case 'q':
	stos[position]=3.0;
	position++;
break;
case 'r':
	stos[position]=4.0;
	position++;
break;
case 's':
	stos[position]=5.0;
	position++;
break;
case 't':
	stos[position]=6.0;
	position++;
break;
case 'u':
	stos[position]=7.0;
	position++;
break;
case 'v':
	stos[position]=8.0;
	position++;
break;
case 'w':
	stos[position]=9.0;
	position++;
break;
case '4':
	stos[position-1]=log(stos[position-1]);
break;
case '5':
	stos[position-1]=exp(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0/(stos[position-1]);
break;
case '9':
	stos[position-1]=tgamma(stos[position-1]);
break;
case 'a':
	stos[position-1]=sqrt(stos[position-1]);
break;
case 'b':
	stos[position-1]=sqr(stos[position-1]);
break;
case 'c':
	stos[position-1]=sin(stos[position-1]);
break;
case 'd':
	stos[position-1]=asin(stos[position-1]);
break;
case 'e':
	stos[position-1]=cos(stos[position-1]);
break;
case 'f':
	stos[position-1]=acos(stos[position-1]);
break;
case 'g':
	stos[position-1]=tan(stos[position-1]);
break;
case 'h':
	stos[position-1]=atan(stos[position-1]);
break;
case 'i':
	stos[position-1]=sinh(stos[position-1]);
break;
case 'j':
	stos[position-1]=asinh(stos[position-1]);
break;
case 'k':
	stos[position-1]=cosh(stos[position-1]);
break;
case 'l':
	stos[position-1]=acosh(stos[position-1]);
break;
case 'm':
	stos[position-1]=tanh(stos[position-1]);
break;
case 'n':
	stos[position-1]=atanh(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=pow(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nan("NAN"));
}
}
return (stos[position-1]);
}
long double constantl(const char * amino, const int aminoLength)
{
long double stos[STACKSIZE];
int position;
int ii;
char instr;
position=0;
for(ii=0;ii<aminoLength;ii++)
{
instr=amino[ii];
switch(instr) {
case '0':
	stos[position]=3.14159265358979323846264338327950288419716939937510582097494459230781640628620899860915591343571l;
	position++;
break;
case '1':
	stos[position]=2.71828182845904523536028747135266249775724709369995957496696762772406399401759425599088053544407l;
	position++;
break;
case '2':
	stos[position]=-1.0l;
	position++;
break;
case '3':
	stos[position]=1.6180339887498948482045868343656381177203091798057628621354486227052604628189l;
	position++;
break;
case 'o':
	stos[position]=1.0l;
	position++;
break;
case 'p':
	stos[position]=2.0l;
	position++;
break;
case 'q':
	stos[position]=3.0l;
	position++;
break;
case 'r':
	stos[position]=4.0l;
	position++;
break;
case 's':
	stos[position]=5.0l;
	position++;
break;
case 't':
	stos[position]=6.0l;
	position++;
break;
case 'u':
	stos[position]=7.0l;
	position++;
break;
case 'v':
	stos[position]=8.0l;
	position++;
break;
case 'w':
	stos[position]=9.0l;
	position++;
break;
case '4':
	stos[position-1]=logl(stos[position-1]);
break;
case '5':
	stos[position-1]=expl(stos[position-1]);
break;
case '8':
	stos[position-1]=1.0l/(stos[position-1]);
break;
case '9':
	stos[position-1]=tgammal(stos[position-1]);
break;
case 'a':
	stos[position-1]=sqrtl(stos[position-1]);
break;
case 'b':
	stos[position-1]=sqrl(stos[position-1]);
break;
case 'c':
	stos[position-1]=sinl(stos[position-1]);
break;
case 'd':
	stos[position-1]=asinl(stos[position-1]);
break;
case 'e':
	stos[position-1]=cosl(stos[position-1]);
break;
case 'f':
	stos[position-1]=acosl(stos[position-1]);
break;
case 'g':
	stos[position-1]=tanl(stos[position-1]);
break;
case 'h':
	stos[position-1]=atanl(stos[position-1]);
break;
case 'i':
	stos[position-1]=sinhl(stos[position-1]);
break;
case 'j':
	stos[position-1]=asinhl(stos[position-1]);
break;
case 'k':
	stos[position-1]=coshl(stos[position-1]);
break;
case 'l':
	stos[position-1]=acoshl(stos[position-1]);
break;
case 'm':
	stos[position-1]=tanhl(stos[position-1]);
break;
case 'n':
	stos[position-1]=atanhl(stos[position-1]);
break;
case '6':
	stos[position-2]=(stos[position-1]+stos[position-2]);
	position--;
break;
case '7':
	stos[position-2]=(stos[position-1]*stos[position-2]);
	position--;
break;
case 'x':
	stos[position-2]=(stos[position-1]-stos[position-2]);
	position--;
break;
case 'y':
	stos[position-2]=(stos[position-1]/stos[position-2]);
	position--;
break;
case 'z':
	stos[position-2]=powl(stos[position-1],stos[position-2]);
	position--;
break;
default : return(nanl("NAN"));
}
}
return (stos[position-1]);
}
