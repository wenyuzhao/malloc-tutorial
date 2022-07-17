#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include "mymalloc.h"

typedef struct Block {
  bool free;
  size_t size;
  struct Block *prev;
  struct Block *next;
} Block;

const size_t kBlockMetadataSize = sizeof(Block);
const size_t kChunkSize = 16ull << 20; // 16MB mmap chunk
const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize;

static const size_t kAlignment = sizeof(size_t); // Word alignment
static const size_t kMinAllocationSize = kAlignment;

static Block *free_head = NULL;

/// Get right neighbour
inline static Block *get_right_block(Block *block) {
  return (Block *)(((size_t)block) + block->size);
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

/// Add block to the freelist
static void add_block(Block *block) {
  assert(!block->free);
  // Set to free
  block->free = true;
  // Add to linked-list
  block->prev = NULL;
  block->next = free_head;
  if (free_head != NULL)
    free_head->prev = block;
  free_head = block;
}

/// Remove block from the freelist
static void remove_block(Block *block) {
  assert(block->free);
  // Set to not free
  block->free = false;
  // Remove from linked-list
  if (block->prev != NULL)
    block->prev->next = block->next;
  if (block->next != NULL)
    block->next->prev = block->prev;
  if (free_head == block) {
    free_head = block->next;
    if (free_head != NULL)
      free_head->prev = NULL;
  }
  block->next = NULL;
  block->prev = NULL;
}

/// Acquire more memory from OS
static Block *acquire_more_memory() {
  // Acquire one more chunk from OS
  Block *block = mmap(NULL, kChunkSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  // Initialize block metadata
  block->free = false;
  block->size = kChunkSize;
  block->prev = NULL;
  block->next = NULL;
  return block;
}

/// Split a block into two. Both of the blocks will be set as non-free
static Block *split(Block *block, size_t size) {
  assert(!block->free);
  size_t total_size = block->size;
  Block *first = block;
  first->free = false;
  first->size = total_size - size - kBlockMetadataSize;
  Block *second = get_right_block(first);
  second->size = total_size - first->size;
  second->free = false;
  second->prev = NULL;
  second->next = NULL;
  return second;
}

void *my_malloc(size_t size) {
  // Round up allocation size
  size = round_up(size, kAlignment);
  if (size == 0 || size > kMaxAllocationSize) return NULL;
  // Find a block in the freelist
  Block *block = NULL;
  for (Block *b = free_head; b != NULL; b = b->next) {
    if (b->size - kBlockMetadataSize >= size) {
      block = b;
      remove_block(block);
      break;
    }
  }
  // Acquire a new chunk from OS if the freelist is empty
  if (block == NULL)
    block = acquire_more_memory();
  // Split block if the block is too large
  if (block->size >= size + (kBlockMetadataSize << 1) + kMinAllocationSize) {
    Block *second = split(block, size);
    Block *first = block;
    add_block(first);
    block = second;
  }
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
  // Add block to freelist
  add_block(block);
}

void verify() {
  for (Block *b = free_head; b != NULL; b = b->next) {
    assert(b->free);
    assert(b->size >= kBlockMetadataSize);
    b->free = false;
  }
  for (Block *b = free_head; b != NULL; b = b->next) {
    assert(!b->free);
    assert(b->size >= kBlockMetadataSize);
    b->free = true;
  }
}
