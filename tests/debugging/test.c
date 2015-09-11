#include <stdio.h>

int f(int a, int b) {
  return a + b;
}

int main() {
  int  a = 1;
  char b = 'b';

  printf("a = %d, b = %c, f() = %d\n", a, b, f(a, b));

  return 0;
}