#include <assert.h>
#include <sys/mman.h>
#include "mymalloc.h"

typedef struct {
  size_t size;
} Chunk;

static const size_t kMetadataSize = sizeof(Chunk); // Size of the chunk metadata
static const size_t kPageSize = 1ull << 12; // Size of a page (4 KB)
const size_t kMaxAllocationSize = (16ULL << 20) - kMetadataSize; // We support allocation up to 16MB

inline static void *get_data(Chunk *chunk) {
  return (void *)(chunk + 1);
}

inline static size_t round_up(size_t size, size_t alignment) {
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

void *my_malloc(size_t size) {
  if (size == 0 || size > kMaxAllocationSize) return NULL;
  // Round up size
  size = round_up(size + kMetadataSize, kPageSize);
  // Request memory from OS
  Chunk *chunk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (chunk == MAP_FAILED) {
    return NULL;
  }
  // Record allocation size
  chunk->size = size;
  // Return data pointer
  LOG("alloc %p size=%zu\n", get_data(chunk), size);
  return get_data(chunk);
}

inline static Chunk *get_chunk(void *ptr) {
  return (Chunk *)(((size_t *)ptr) - 1);
}

void my_free(void *ptr) {
  LOG("free %p\n", ptr);
  if (ptr == NULL) return;
  // Get chunk
  Chunk *chunk = get_chunk(ptr);
  // Unmap memory
  munmap(chunk, chunk->size);
}

void verify() {} // Used for heap verification
