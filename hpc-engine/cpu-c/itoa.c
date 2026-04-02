#include <math.h>
#include "itoa.h"
const char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";

unsigned long long int ipow(int b, int n)
{
  int k;
  unsigned long long int res;
  
  res=1;
  
  for(k=0;k<n;k++) res = res*b;
  
  return res;
}

int ilog(int base, unsigned long long int n)
{
  
  if(n>0) 
    return (int) -0.5 + (log((double) n)/log( (double) base));
  else 
    return 0;
}

/* http://www.jb.man.ac.uk/~slowe/cpp/itoa.html */	
void itoa(unsigned long long int value, char* str, const int base, const int padding) {
	
	//const char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	//char* wstr=str;
	int k;
	
	// padding with zeros
	for(k=0;k<padding;k++) *(str+k) = '0';
	*(str+padding) = '\0';
	
	// Conversion. Number is reversed.
	
	do *str++ = num[value%base]; while(value/=base);
	
//	if(sign<0) *wstr++='-';
	
	//*wstr='\0';
}

void itoaFAST(unsigned int value, char* str, unsigned const int base, unsigned const int padding) {
	
	//const char num[] = "0123";//456789abcdefghijklmnopqrstuvwxyz";
	
	char* wstr=str;
	int k;

		// padding with zeros
	for(k=0;k<padding;k++) *(wstr+k) = '0';
	*(wstr+padding) = '\0';
	
	
	// Conversion. Number is reversed.
	
	do *wstr++ = num[value%base]; while(value/=base);

	
}


void itoa_update(char *str, const int base, const int padding) {
	

  for(int i=0;i<padding;i++)
  {

    if ( str[i] != (num[base-1]) ) {
      if (str[i] <= '9') {
       str[i] = num[str[i] - '0' + 1];
      } else {
       str[i] = num[str[i] - 'a' + 11];
      }
	  return;
    } else {
      str[i]=num[0];
    }
  
  }
  
}




