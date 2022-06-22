#include "utils.h"

static const size_t kMinAlignment = 8;

static char heap[4096];
static size_t cursor = 0;

void* malloc(size_t size) {
  // Align pointer
  cursor = (cursor + (kMinAlignment - 1)) & ~(kMinAlignment - 1);
  // Get result pointer
  void* ptr = (void*) &heap[cursor];
  // Update cursor
  cursor += size;
  // Log
  LOG("allocate %p size=%zu\n", ptr, size);
  return ptr;
}

void free(void* ptr) {
  // Do nothing
  USE(ptr);
}
