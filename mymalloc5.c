#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mymalloc.h"

typedef struct Block
{
  size_t size : 32;
  size_t left_size : 31;
  size_t free : 1;
  struct Block *prev;
  struct Block *next;
} Block;

const size_t kBlockMetadataSize = sizeof(Block);
const size_t kBlockFixedMetadataSize = offsetof(Block, prev);
const size_t kChunkSize = 16ull << 20; // 16MB mmap chunk
const size_t kFenceSize = sizeof(size_t);
const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize - (kFenceSize << 1); // We support allocation up to ~16MB

static const size_t kAlignment = sizeof(size_t); // Word alignment
static const size_t kMinAllocationSize = kAlignment;
static const size_t kFenceValue = 0xdeadbeef;

static Block *lists[N_LISTS + 1];

static void *top = NULL;
static Block *top_block = NULL;

static void *bottom = NULL;
static Block *bottom_block = NULL;

inline static size_t max(size_t a, size_t b)
{
  return a >= b ? a : b;
}

/// Get size class
inline static size_t size_class(size_t size)
{
  assert(size >= kAlignment);
  size_t sc = (size / kAlignment) - 1;
  return sc >= N_LISTS ? N_LISTS : sc;
}

/// Get right neighbour
inline static Block *get_right_block(Block *block)
{
  return (Block *)(((size_t)block) + block->size);
}

/// Get left neighbour
inline static Block *get_left_block(Block *block)
{
  return (Block *)(((size_t)block) - block->left_size);
}

/// Round up size
inline static size_t size_align_up(size_t size, size_t alignment)
{
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

/// Get data pointer of a block
inline static void *block_to_data(Block *block)
{
  return (void *)(((size_t)block) + kBlockFixedMetadataSize);
}

/// Get the block of the data pointer
inline static Block *data_to_block(void *ptr)
{
  return (Block *)(((size_t)ptr) - kBlockFixedMetadataSize);
}

/// Add block to the freelist
static void add_block(Block *block)
{
  assert(block->size >= kBlockMetadataSize);
  size_t sc = size_class(block->size - kBlockFixedMetadataSize);
  block->prev = NULL;
  block->next = lists[sc];
  if (lists[sc] != NULL)
    lists[sc]->prev = block;
  lists[sc] = block;
}

/// Remove block from the freelist
static void remove_block(Block *block)
{
  assert(block->size >= kBlockMetadataSize);
  size_t sc = size_class(block->size - kBlockFixedMetadataSize);
  if (block->prev != NULL)
    block->prev->next = block->next;
  if (block->next != NULL)
    block->next->prev = block->prev;
  if (lists[sc] == block)
  {
    lists[sc] = block->next;
    if (lists[sc] != NULL)
      lists[sc]->prev = NULL;
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
  block->free = false;
  block->size = kChunkSize - (kFenceSize << 1);
  block->left_size = kFenceSize;
  block->prev = NULL;
  block->next = NULL;
  void *end = (void *)(((size_t)ptr) + kChunkSize);
  // Try merge bottom chunks
  if (bottom != NULL && bottom == end)
  {
    // Merge chunks
    assert(is_fence(get_left_block(bottom_block)));
    if (bottom_block->free)
    {
      remove_block(bottom_block);
      block->size = bottom_block->size + kChunkSize;
      Block *right = get_right_block(bottom_block);
      right->left_size = block->size;
    }
    else
    {
      block->size = kChunkSize;
      bottom_block->left_size = kChunkSize;
    }
  }
  // Update bottom cursor
  if (bottom == NULL || (size_t)ptr < (size_t)bottom)
  {
    bottom = (void *)ptr;
    bottom_block = block;
  }
  // Try merge top chunks
  if (top != NULL && top == ptr)
  {
    // Merge chunks
    Block *right = get_right_block(top_block);
    assert(is_fence(right));
    if (top_block->free)
    {
      remove_block(top_block);
      top_block->free = false;
      top_block->size += kChunkSize;
      top_block->prev = NULL;
      top_block->next = NULL;
      block = top_block;
    }
    else
    {
      right->free = false;
      right->size = kChunkSize;
      right->left_size = top_block->size;
      right->prev = NULL;
      right->next = NULL;
      block = right;
    }
  }
  // Update top cursor
  if ((size_t)ptr > (size_t)top)
  {
    top = (void *)(((size_t)ptr) + kChunkSize);
    top_block = block;
  }
  return block;
}

/// Split a block into two
static Block *split(Block *block, size_t size)
{

  // Split block
  size_t total_size = block->size;
  Block *first = block;
  first->free = true;
  first->size = total_size - max(size + kBlockFixedMetadataSize, kBlockMetadataSize);
  assert(first->size >= kMinAllocationSize);
  Block *second = get_right_block(first);
  second->size = total_size - first->size;
  second->left_size = first->size;
  second->free = false;
  second->prev = NULL;
  second->next = NULL;
  assert(first != second);
  // Update the right neighbour of the second block
  Block *right = get_right_block(second);
  if (!is_fence(right))
    right->left_size = second->size;
  // Update top block
  if (block == top_block)
    top_block = second;
  return second;
}

/// Try allocate from the last general freelist
static Block *alloc_from_general_list(size_t alloc_size)
{
  Block *block = NULL;
  for (Block *b = lists[N_LISTS]; b != NULL; b = b->next)
  {
    if (b->size - kBlockFixedMetadataSize >= alloc_size)
    {
      block = b;
      remove_block(block);
      break;
    }
  }
  if (block == NULL)
    block = acquire_more_memory(alloc_size);
  assert(block != NULL);
  block->free = false;
  block->next = NULL;
  block->prev = NULL;
  assert(block->size >= alloc_size + kBlockFixedMetadataSize);
  return block;
}

/// Allocate from one of the freelists
static Block *alloc_with_size_class(size_t sc, size_t alloc_size)
{
  if (lists[sc] != NULL && sc < N_LISTS)
  {
    // Current list is not empty
    Block *block = lists[sc];
    lists[sc] = lists[sc]->next;
    if (lists[sc] != NULL)
      lists[sc]->prev = NULL;
    block->free = false;
    block->next = NULL;
    block->prev = NULL;
    assert(block->size >= alloc_size + kBlockFixedMetadataSize);
    return block;
  }
  else
  {
    Block *block = sc < N_LISTS ? alloc_with_size_class(sc + 1, alloc_size) : alloc_from_general_list(alloc_size);
    if (block->size >= alloc_size + (kBlockMetadataSize << 1) + kMinAllocationSize)
    {
      Block *second = split(block, alloc_size);
      Block *first = block;
      add_block(first);
      block = second;
      assert(block->size >= alloc_size + kBlockFixedMetadataSize);
    }
    assert(block->size >= alloc_size + kBlockFixedMetadataSize);
    assert(!block->free);
    return block;
  }
}

void *my_malloc(size_t size)
{
  // Round up allocation size
  size = size_align_up(size, kAlignment);
  if (size == 0 || size > kMaxAllocationSize)
    return NULL;
  size_t sc = size_class(size);
  // Try pop a block from list
  Block *block = alloc_with_size_class(sc, size);
  // Zero memory and return
  void *data = block_to_data(block);
  assert(block->size >= size + kBlockFixedMetadataSize);
  memset(data, 0, size);
  LOG("alloc %p size=%zu block=%p\n", data, size, (void *)block);
  return data;
}

/// Coalesce two neighbour blocks
static void coalesce_blocks(Block *left, Block *right)
{
  assert(right == get_right_block(left));
  // Remove left from the list
  remove_block(left);
  // Remove right from the list
  remove_block(right);
  // Merge left and right
  left->size += right->size;
  // Update right.right block
  Block *right_right = get_right_block(right);
  if (!is_fence(right_right))
    right_right->left_size = left->size;
  // Add left back to list
  add_block(left);
  // Update top block
  if (right == top_block)
    top_block = left;
}

void my_free(void *ptr)
{
  if (ptr == NULL)
    return;
  Block *block = data_to_block(ptr);
  LOG("free %p size=%zu block=%p\n", ptr, block->size - kBlockFixedMetadataSize, (void *)block);
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
  Block *left = get_left_block(block);
  if (!is_fence(left) && left->free)
    coalesce_blocks(left, block);
}
