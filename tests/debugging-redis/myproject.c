#include <stdio.h>
#include <stdlib.h>

int f(int a, int b) {
  return a + b;
}

int main() {
  int  a = 1;
  char b = 'b';

  fprintf(stderr, "a = %d, b = %c, f() = %d\n", a, b, f(a, b));

  fprintf(stderr, "some output\n");

  return -1;
}
