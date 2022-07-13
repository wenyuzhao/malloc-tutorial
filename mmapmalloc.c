#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include "mymalloc.h"

typedef struct
{
  size_t size;
} Chunk;

const size_t kChunkSize = 1ull << 12;               // Size of a page (4 KB)
static const size_t kMetadataSize = sizeof(size_t); // Allocation size metadata

inline static Chunk *get_chunk(void *ptr)
{
  return (Chunk *)(((size_t *)ptr) - 1);
}

inline static void *get_data(Chunk *chunk)
{
  return (void *)(((size_t *)chunk) + 1);
}

inline static size_t size_align_up(size_t size, size_t alignment)
{
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

void *my_malloc(size_t size)
{
  // Round up size
  size_t chunk_size = size_align_up(size + kMetadataSize, kChunkSize);
  // Request memory from OS
  Chunk *chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  assert(chunk != MAP_FAILED);
  // Update metadata
  chunk->size = chunk_size;
  // Return data pointer
  void *data = get_data(chunk);
  LOG("alloc %p size=%zu\n", data, size);
  return data;
}

void my_free(void *ptr)
{
  LOG("free %p\n", ptr);
  // Get chunk
  Chunk *chunk = get_chunk(ptr);
  // Unmap memory
  munmap(chunk, chunk->size);
}

void verify() {}
