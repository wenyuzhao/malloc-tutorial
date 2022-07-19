# Lab 5: Implement `malloc` and `free`

# <span style="color: red">Before releasing the lab, please remove all ***mymalloc\*.c*** files in the repo!</span>

# 0. Introduction

In this lab you will build a malloc implementation from scratch. We will start from building a very simple allocator, the mmap allocator, which simply indirects all the allocation and deallocation requests to `mmap` and `munmap` calls. Based on the mmap allocator, we will then build a simple single freelist allocator that manages the memory ourselves.

The header file _mymalloc.h_ in the repo provides the signature of the malloc and free functions. Please note here we renamed them to `my_malloc` and `my_free`, because extra difficulties would be involved if debugging a library with overridden `malloc` and `free` implementations.

Just like `malloc` / `free`,
  * `void* my_malloc(size_t size)` is called when the user program wants to allocate some memory. The argument is the size (in bytes) of the allocation. The function should return a pointer pointing to the start of the allocated memory. The result start address should be **word aligned (8 bytes on 64-bit machines)**.
  * `void free(void* ptr)` is for releasing a previously allocated memory. The argument is the start address of the allocated memory.

## The `size_t` type
You may have noticed the `size_t` type being used in the function declaration of `my_malloc`. As you may remember from a previous (**NOTE CHECK THIS IS TRUE**) lab, the sizes of data types in C are often implementation defined. Imagine if an `int` could only store 16-bits of information, then we would not be able to represent the value 100,000 (and more relevantly could not allocate large arrays)! The C specification, hence, describes the `size_t` type in order to represent the size of any object (including arrays). In other words: `size_t` is defined to be "how many bytes it takes to reference any location in memory". Hence, the size of `size_t` is defined to be the same size as the CPU's architecture, with a 32-bit architecture having a `size_t` of 4 bytes (32-bits) and a 64-bit architecture having a `size_t` of 8 bytes (64-bits). The `sizeof()` operator that you have seen being used throughout returns a value of type `size_t`.

The C specification defines another similar type: the `ptrdiff_t` type. This type has the same size as `size_t` but is used to represent the difference between two pointers _of the same types_.

For more information about `size_t` and `ptrdiff_t` you can run `man size_t` or `man ptrdiff_t` in your terminals. Now let's get back to the lab.

In this lab, you will be required to provide your own implementation to these two functions. We provide a set of tests with a testing script to check the correctness of your implementation.

For your development, please use a Linux distribution or MacOS with `clang` installed.

**Note:** Please create an empty `mymalloc.c` file in this repo. We will implement the allocators totally in this file.

# 1. A mmap-based allocator

This mmap-based allocator simply directs the `my_malloc` and `my_free` calls to `mmap` / `munmap`.

## Getting started

We first create a `mymalloc.c` file in the repo, containing all the necessary definitions for our implementation:

```c
// mymalloc.c
#include "mymalloc.h"

const size_t kMaxAllocationSize = 0;

void *my_malloc(size_t size) {
  return NULL;
}

void my_free(void *ptr) {
}

void verify() {} // Used for heap verification
```

## Allocate memory

For this mmap allocator, if `my_malloc` is called to allocate $N$ bytes, we directly call `mmap` to allocate a chunk with size _no less than_ $N$ bytes. Each `mmap`-ed chunk has a piece of metadata memory for us to record the size of the chunk. So later when we call `munmap`, we will always know the size of the `mmap`-ed chunk. Why is this metadata important? Discuss amongst yourselves what would happen if this metadata is omitted.

We first define the `Chunk` data structure, and set an appropriate max allocation size we're going to support, say, 16 MB.

Then, we delegate the allocation to `mmap`:

```diff
  // mymalloc.c
+ #include <sys/mman.h>
  #include "mymalloc.h"
+
+ typedef struct {
+   size_t size;
+ } Chunk;

+ static const size_t kMetadataSize = sizeof(Chunk); // Size of the chunk metadata
- const size_t kMaxAllocationSize = 0;
+ const size_t kMaxAllocationSize = (16ULL << 20) - kMetadataSize; // We support allocation up to 16MB

+ inline static void *get_data(Chunk *chunk) {
+   return (void *)(chunk + 1);
+ }
+
  void *my_malloc(size_t size) {
-   return NULL;
+   // Request memory from OS
+   Chunk *chunk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
+   // Record allocation size
+   chunk->size = size;
+   // Return data pointer
+   return get_data(chunk);
  }
```

**Note:** As you have learnt previously, `mmap` can fail! What should you return in the case when `mmap` fails to allocate more memory? How can we inform a calling function that we're out of memory? ***NOTE: CHECK IF STUDENTS KNOW `errorno`***.


## Metadata reservation and mmap size

Note that:
 1. The chunk metadata requires extra space in addition to the allocation size.
 2. The size passed to `mmap` must be a multiple of page size (4096 bytes). Our `my_malloc` implementation is not fully correct yet.

To solve these issues, we first add the allocation size by the chunk metadata size, and then round it up to a multiple of page size.

```diff
  static const size_t kMetadataSize = sizeof(Chunk); // Size of the chunk metadata.
+ static const size_t kPageSize = 1ull << 12; // Size of a page (4 KB)
  const size_t kMaxAllocationSize = 16 * 1024 * 1024 - kMetadataSize; // We support allocation up to 16MB

  inline static void *get_data(Chunk *chunk) {
    return (void *)(chunk + 1);
  }

+ inline static size_t round_up(size_t size, size_t alignment) {
+   const size_t mask = alignment - 1;
+   return (size + mask) & ~mask;
+ }
+
  void *my_malloc(size_t size) {
+   // Round up size
+   size = round_up(size + kMetadataSize, kPageSize);
    // Request memory from OS
    Chunk *chunk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
```

## Free Memory

For `my_free`, we first get the `Chunk*` pointer by subtracting the data pointer by the metadata size. We then fetch the chunk size from the metadata memory, and call `munmap` to free the memory.

```diff
  }

+ inline static Chunk *get_chunk(void *ptr) {
+   return (Chunk *)(((size_t *)ptr) - 1);
+ }

  void my_free(void *ptr) {
+   // Get chunk
+   Chunk *chunk = get_chunk(ptr);
+   // Unmap memory
+   munmap(chunk, chunk->size);
  }
```

## Corner cases

There're a few corner cases that need to resolve for completeness:

* For allocation with zero size or greater than the `max_allocation_size()`, `my_malloc` should return `NULL`.
* Free a `NULL` pointer is a no-op.

```diff
  void *my_malloc(size_t size) {
+   if (size == 0 || size > kMaxAllocationSize) return NULL;
    // Round up size
    size = round_up(size + kMetadataSize, kPageSize);

  ...

  void *my_malloc(size_t size) {
+   if (ptr == NULL) return;
    // Get chunk
    Chunk *chunk = get_chunk(ptr);
```

## Put all together

```c
// mymalloc.c
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
  // Record allocation size
  chunk->size = size;
  // Return data pointer
  return get_data(chunk);
}

inline static Chunk *get_chunk(void *ptr) {
  return (Chunk *)(((size_t *)ptr) - 1);
}

void my_free(void *ptr) {
  if (ptr == NULL) return;
  // Get chunk
  Chunk *chunk = get_chunk(ptr);
  // Unmap memory
  munmap(chunk, chunk->size);
}

void verify() {} // Used for heap verification
```

## Run the tests

After finishing `my_malloc` and `my_free`, please use the testing script to run the tests, to check the correctness of your code:

```console
$ ./test.py
```

**_Other usages:_**

* Run a single test: `./test.py -t all_lists`.
* Run tests with release build:  `./test.py --release`.
* Enable logging:  `./test.py --log`.
  * Please use `LOG("Hello %s\n", "world");` for logging in your source code.
* If your source file is called *different_malloc.c* instead of _mymalloc.c_: `./test.py -m different_malloc`.

### Failing tests

You may have noticed that a few tests fail with your mmap allocator. This is deliberate and in fact a major limitation of the naïve mmap allocator. Allocating 4096 bytes if you only need 16 bytes is wasteful! We can quickly run out of memory if we allocate a few objects. The failing tests in fact simulate a low memory system and fail if too much memory has been allocated. Discuss amongst yourselves on what other limitations does the mmap allocator have (_hint: think about the performance!_).

# 2. Simple freelist allocator

Most of the user allocations are smaller than a page size. Always using `mmap` for allocation can significantly waste space. Also, letting OS to manage the memory for us is less efficient, because we need to do a expensive syscall for every allocation.

We will now use a global freelist to manage the memory on our own.

Same as before, we will start from [an empty implementation file](#getting-started) and name it as _mymalloc.c_.

## Freelist operations

## Allocate memory from the global freelist

## Acquire more memory from OS

## Release memory to the global freelist

## Testing

Don't forget to run the tests after the implementation is done: `./test.py`.

## Debugging using GDB

## Heap verification

## Put all together

```c
// mymalloc.c
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
```
