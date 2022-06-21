#include <stdio.h>

static const size_t kMinAlignment = 8;

static char heap[4096];
static size_t cursor = 0;

void* malloc(size_t size) {
  cursor = (cursor + (kMinAlignment - 1)) & !(kMinAlignment - 1);
  void* ptr = (void*) &heap[cursor];
  cursor += size;
  return ptr;
}

void free() {}
