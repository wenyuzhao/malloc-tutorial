#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

typedef struct Block
{
  bool free;
  size_t size;
  struct Block *prev;
  struct Block *next;
} Block;

const size_t kBlockMetadataSize = sizeof(Block);
const size_t kChunkSize = 1ull << 12; // Size of a page (4 KB)
const size_t kFenceSize = sizeof(size_t);

static const size_t kAlignment = sizeof(size_t); // Word alignment
static const size_t kMinAllocationSize = kAlignment;
static const size_t kFenceValue = 0xdeadbeef;

static Block *free_head = NULL;
static size_t allocated = 0;

/// Get right neighbour
inline static Block *get_right_block(Block *block)
{
  return (Block *)(((size_t)block) + block->size);
}

/// Round up size
inline static size_t size_align_up(size_t size, size_t alignment)
{
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

/// Add block to the freelist
static void add_block(Block *block)
{
  block->prev = NULL;
  block->next = free_head;
  if (free_head != NULL)
    free_head->prev = block;
  free_head = block;
}

/// Remove block from the freelist
static void remove_block(Block *block)
{
  if (block->prev != NULL)
    block->prev->next = block->next;
  if (block->next != NULL)
    block->next->prev = block->prev;
  if (free_head == block)
  {
    free_head = block->next;
    if (free_head != NULL)
      free_head->prev = NULL;
  }
  block->next = NULL;
  block->prev = NULL;
}

/// Check if we're touching a fence
inline static bool is_fence(Block *block)
{
  return *((size_t *)block) == kFenceValue;
}

/// Acquire more memory from OS
static Block *acquire_more_memory(size_t alloc_size)
{
  assert(alloc_size + kBlockMetadataSize + (kFenceSize << 1) <= kChunkSize);
  // Acquire one more chunk from OS
  size_t *ptr = mmap(NULL, kChunkSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  assert(ptr != MAP_FAILED);
  // Mark fences
  *ptr = kFenceValue;
  *((size_t *)(((size_t)ptr) + kChunkSize - kFenceSize)) = kFenceValue;
  // Initialize block metadata
  Block *block = (Block *)(ptr + 1);
  block->free = true;
  block->size = kChunkSize - (kFenceSize << 2);
  block->prev = NULL;
  block->next = NULL;
  // Add block to list
  add_block(block);
  return block;
}

void *my_malloc(size_t size)
{
  // Round up allocation size
  size = size_align_up(size, kAlignment);
  if (size == 0 || size > max_allocation_size())
    return NULL;
  // Find a block
  Block *block = NULL;
  for (Block *b = free_head; b != NULL; b = b->next)
  {
    if (b->size - kBlockMetadataSize >= size)
    {
      block = b;
      break;
    }
  }
  if (block == NULL)
    block = acquire_more_memory(size);
  assert(block != NULL);
  // Remove block from list
  remove_block(block);
  // Update block metadata
  if (block->size < size + (kBlockMetadataSize << 1) + kMinAllocationSize)
  {
    // Mark block as allocated
    block->free = false;
  }
  else
  {
    // Split block
    size_t total_size = block->size;
    Block *first = block;
    first->free = true;
    first->size = total_size - size - kBlockMetadataSize;
    assert(first->size >= kMinAllocationSize);
    Block *second = get_right_block(first);
    second->size = total_size - first->size;
    second->free = false;
    second->prev = NULL;
    second->next = NULL;
    assert(first != second);
    // Use right block for allocation
    add_block(first);
    block = second;
  }
  // Update allocation counter
  allocated += size;
  // Zero memory and return
  void *data = (void *)(((size_t)block) + kBlockMetadataSize);
  memset(data, 0, size);
  LOG("alloc %p size=%zu block=%p\n", data, size, (void *)block);
  return data;
}

/// Coalesce two neighbour blocks
static void coalesce_blocks(Block *left, Block *right)
{
  // Remove right from the list
  remove_block(right);
  // Merge left and right
  left->size += right->size;
}

void my_free(void *ptr)
{
  if (ptr == NULL)
    return;
  Block *block = (Block *)(((size_t)ptr) - kBlockMetadataSize);
  LOG("free %p size=%zu block=%p\n", ptr, block->size, (void *)block);
  assert(!block->free);
  block->free = true;
  // Add block to freelist
  add_block(block);
  // Try coalescing
  // 1. Merge with right neighbour
  Block *right = get_right_block(block);
  if (!is_fence(right) && right->free)
    coalesce_blocks(block, right);
  // 2. Merge with left neighbour
  for (Block *b = free_head; b != NULL; b = b->next)
  {
    Block *right = get_right_block(b);
    if (!is_fence(right) && right->free && right == block)
    {
      coalesce_blocks(b, block);
      break;
    }
  }
}
