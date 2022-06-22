#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include "mymalloc.h"

typedef struct Block
{
  bool free;
  size_t size;
  size_t left_size;
  struct Block *prev;
  struct Block *next;
} Block;

static const size_t kAlignment = sizeof(size_t); // Word alignment
static const size_t kChunkSize = 1ull << 12;     // Size of a page (4 KB)
static const size_t kBlockMetadataSize = sizeof(Block);
static const size_t kFenceSize = sizeof(size_t);
static const size_t kFenceValue = 0xdeadbeef;

static Block *free_head = NULL;
static size_t allocated = 0;

inline static size_t size_align_up(size_t size, size_t alignment)
{
  const size_t mask = alignment - 1;
  return (size + mask) & ~mask;
}

static Block *request_more_memory(size_t alloc_size)
{
  size_t chunk_size = size_align_up(alloc_size + kBlockMetadataSize + (kFenceSize << 1), kChunkSize);
  // Request more memory from OS
  size_t *ptr = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  assert(ptr != MAP_FAILED);
  // Mark fences
  *ptr = kFenceValue;
  *((size_t *)(((size_t)ptr) + chunk_size - kFenceSize)) = kFenceValue;
  // Initialize block metadata
  Block *block = (Block *)(ptr + 1);
  block->free = true;
  block->size = chunk_size - (kFenceSize << 2);
  block->left_size = kFenceSize;
  block->prev = NULL;
  block->next = NULL;
  // Add block to list
  block->prev = NULL;
  block->next = free_head;
  if (free_head != NULL)
    free_head->prev = block;
  free_head = block;
  return block;
}

void *my_malloc(size_t size)
{
  // Round up allocation size
  size = size_align_up(size, kAlignment);
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
    block = request_more_memory(size);
  assert(block != NULL);
  // Update block metadata
  if (block->size - (kBlockMetadataSize << 1) == size)
  {
    // Mark block as allocated
    block->free = false;
  }
  else
  {
    // Split block
    size_t total_size = block->size;
    Block *first = block;
    first->size = total_size - size - kBlockMetadataSize;
    Block *second = (Block *)(((size_t)block) + first->size);
    second->size = total_size - first->size;
    second->left_size = first->size;
    second->free = false;
    // Use right block for allocation
    block = second;
  }
  // Update allocation counter
  allocated += size;
  // Return
  void *data = (void *)(((size_t)block) + kBlockMetadataSize);
  LOG("alloc %p size=%zu block=%p\n", data, size, (void *)block);
  return data;
}

inline static bool is_fence(Block *block)
{
  return *((size_t *)block) == kFenceValue;
}

static void coalesce_blocks(Block *left, Block *right)
{
  // Remove right from the list
  if (right->prev != NULL)
    right->prev->next = right->next;
  if (right->next != NULL)
    right->next->prev = right->prev;
  if (right == free_head)
  {
    free_head = right->next;
    free_head->prev = NULL;
  }
  // Merge left and right
  left->size += right->size;
  // Update right.right block
  Block *right_right = (Block *)(((size_t)right) + right->size);
  if (!is_fence(right_right))
    right_right->left_size = left->size;
}

void my_free(void *ptr)
{
  Block *block = (Block *)(((size_t)ptr) - kBlockMetadataSize);
  LOG("free %p size=%zu block=%p\n", ptr, block->size, (void *)block);
  block->free = false;
  // Add block to freelist
  block->prev = NULL;
  block->next = free_head;
  if (free_head != NULL)
    free_head->prev = block;
  free_head = block;
  // Try coalescing
  Block *right = (Block *)(((size_t)block) + block->size);
  if (!is_fence(right) && right->free)
    coalesce_blocks(block, right);
  Block *left = (Block *)(((size_t)block) - block->left_size);
  if (!is_fence(left) && left->free)
    coalesce_blocks(left, block);
}
