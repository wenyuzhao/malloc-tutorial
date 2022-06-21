#include <stdio.h>
#include <stdlib.h>

int main() {
  void* ptr = malloc(123);
  printf("Allocate: %p\n", ptr);
  free(ptr);
  printf("%p is freed\n", ptr);
  return 0;
}
