#include <stdio.h>

void *malloc(size_t size) {
  printf("Allocate %zu bytes\n", size);
  return NULL;
}
