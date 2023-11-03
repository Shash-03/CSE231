#include <stdio.h>
#include <stdlib.h>

long long fib(int n) {
  if(n<2) {
    return n;
  }
  else {
    return fib(n-1)+fib(n-2);
  }
}

int main(){
	long long val = fib(43);
	printf("%lld\n",val);
  
}