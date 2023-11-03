#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "dummy_main.h"

long long fib(int n) {
  if(n<2) {
    return n;
  }
  else {
    return fib(n-1)+fib(n-2);
  }
}



int main(int argc, char* argv[]){
  
	long long val = fib(45);
	printf("%lld\n",val);
  
}