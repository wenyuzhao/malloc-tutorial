
#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include "mymalloc.h"

typedef struct Block {
  bool free;
  size_t size;
} Block;

const size_t kBlockMetadataSize = sizeof(Block);
const size_t kChunkSize = 16ull << 20; // 16MB mmap chunk
const size_t kMemorySize = kChunkSize << 2; // 64MB mmap chunk
const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize;

static const size_t kAlignment = sizeof(size_t); // Word alignment
static const size_t kMinAllocationSize = kAlignment;

static Block *start = NULL;

/// Get right neighbour
inline static Block *get_right_block(Block *block) {
  size_t ptr = ((size_t) block) + block->size;
  if (ptr >= ((size_t) start) + kMemorySize) return NULL;
  return (Block*) ptr;
}

/// Round up size
inline static size_t round_up(size_t size, size_t alignment) {
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

/// Get data pointer of a block
inline static void *block_to_data(Block *block) {
  return (void *)(((size_t)block) + kBlockMetadataSize);
}

/// Get the block of the data pointer
inline static Block *data_to_block(void *ptr) {
  return (Block *)(((size_t)ptr) - kBlockMetadataSize);
}

/// Acquire more memory from OS
static Block *acquire_memory() {
  // Acquire one more chunk from OS
  Block *block = mmap(NULL, kMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  // Initialize block metadata
  block->free = true;
  block->size = kMemorySize;
  return block;
}

/// Split a block into two. Both of the blocks will be set as non-free
static Block *split(Block *block, size_t size) {
  assert(block->free);
  size_t total_size = block->size;
  Block *first = block;
  first->free = false;
  first->size = total_size - size - kBlockMetadataSize;
  Block *second = get_right_block(first);
  second->size = total_size - first->size;
  second->free = false;
  return second;
}

void *my_malloc(size_t size) {
  // Round up allocation size
  // size = round_up(size, kAlignment);
  if (size == 0 || size > kMaxAllocationSize) return NULL;
  // Initial allocation?
  if (start == NULL) {
    start = acquire_memory();
  }
  // Find a block in the freelist
  Block *block = NULL;
  for (Block *b = start; b != NULL; b = get_right_block(b)) {
    if (b->free && b->size - kBlockMetadataSize >= size) {
      block = b;
      break;
    }
  }
  // Split block if the block is too large
  if (block->size >= size + (kBlockMetadataSize << 1) + kMinAllocationSize) {
    Block *second = split(block, size);
    Block *first = block;
    first->free = true;
    block = second;
  }
  // Mark block as allocated
  block->free = false;
  // Zero memory and return
  void *data = block_to_data(block);
  memset(data, 0, size);
  assert(!block->free);
  return data;
}

void my_free(void *ptr) {
  if (ptr == NULL) return;
  // Get block pointer
  Block *block = data_to_block(ptr);
  assert(!block->free);
  // Mark block as free
  block->free = true;
}
