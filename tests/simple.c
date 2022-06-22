#include <stdio.h>
#include <stdlib.h>
#include "../mymalloc.h"

int main()
{
  void *ptr = my_malloc(123);
  printf("Allocate: %p\n", ptr);
  my_free(ptr);
  printf("%p is freed\n", ptr);
  return 0;
}
