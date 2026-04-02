
#include <string.h>
#include <stdio.h>


void print_code_mathematica(const char * amino, const int aminoLength, char *RPN_full_Code)
{


  
int ii;
char instr;

RPN_full_Code[0] = '\0';

  for(ii=0;ii<aminoLength;ii++)
   {
	instr = amino[ii];
    switch(instr)
	 {
case '0':
strcat(RPN_full_Code,"PI, ");
break;
case '1':
strcat(RPN_full_Code,"EULER, ");
break;
case '2':
strcat(RPN_full_Code,"NEG, ");
break;
case '3':
strcat(RPN_full_Code,"GOLDENRATIO, ");
break;
case '4':
strcat(RPN_full_Code,"LOG, ");
break;
case '5':
strcat(RPN_full_Code,"EXP, ");
break;
case '6':
strcat(RPN_full_Code,"PLUS, ");
break;
case '7':
strcat(RPN_full_Code,"TIMES, ");
break;
case '8':
strcat(RPN_full_Code,"INV, ");
break;
case '9':
strcat(RPN_full_Code,"GAMMA, ");
break;
case 'a':
strcat(RPN_full_Code,"SQRT, ");
break;
case 'b':
strcat(RPN_full_Code,"SQR, ");
break;
case 'c':
strcat(RPN_full_Code,"SIN, ");
break;
case 'd':
strcat(RPN_full_Code,"ARCSIN, ");
break;
case 'e':
strcat(RPN_full_Code,"COS, ");
break;
case 'f':
strcat(RPN_full_Code,"ARCCOS, ");
break;
case 'g':
strcat(RPN_full_Code,"TAN, ");
break;
case 'h':
strcat(RPN_full_Code,"ARCTAN, ");
break;
case 'i':
strcat(RPN_full_Code,"SINH, ");
break;
case 'j':
strcat(RPN_full_Code,"ARCSINH, ");
break;
case 'k':
strcat(RPN_full_Code,"COSH, ");
break;
case 'l':
strcat(RPN_full_Code,"ARCCOSH, ");
break;
case 'm':
strcat(RPN_full_Code,"TANH, ");
break;
case 'n':
strcat(RPN_full_Code,"ARCTANH, ");
break;
case 'o':
strcat(RPN_full_Code,"ONE, ");
break;
case 'p':
strcat(RPN_full_Code,"TWO, ");
break;
case 'q':
strcat(RPN_full_Code,"THREE, ");
break;
case 'r':
strcat(RPN_full_Code,"FOUR, ");
break;
case 's':
strcat(RPN_full_Code,"FIVE, ");
break;
case 't':
strcat(RPN_full_Code,"SIX, ");
break;
case 'u':
strcat(RPN_full_Code,"SEVEN, ");
break;
case 'v':
strcat(RPN_full_Code,"EIGHT, ");
break;
case 'w':
strcat(RPN_full_Code,"NINE, ");
break;
case 'x':
strcat(RPN_full_Code,"SUBTRACT, ");
break;
case 'y':
strcat(RPN_full_Code,"DIVIDE, ");
break;
case 'z':
strcat(RPN_full_Code,"POWER, ");
break;
}}
    size_t len = strlen(RPN_full_Code);
    if (len > 2) {
        RPN_full_Code[len - 2] = '\0';
    }
}