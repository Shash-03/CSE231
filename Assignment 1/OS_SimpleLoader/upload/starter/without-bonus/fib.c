/*
 * No changes are allowed in this file
 */
int fib(int n) {
  if (n == 1 ){
    return 1;
  }
  else if (n == 2){
    return 1;
  }
  else{
    return fib(n-1) + fib(n-2);
  }
}

int _start() {
	int val = fib(40);
	return val;
}
