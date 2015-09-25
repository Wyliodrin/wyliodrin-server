#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

bool _sanity_check(char *file, int line, int num_params, ...) {
  bool ret = true;

  va_list ap;
  va_start(ap, num_params);

  int i;
  for (i = 0; i < num_params; i++) {
    if (va_arg(ap, int) == 0) {
      fprintf(stderr, "[SANITY_CHECK %s:%d] Condition %d failed\n", file, line, i);
      ret = false;
    }
  }

  va_end(ap);

  return ret;
}
