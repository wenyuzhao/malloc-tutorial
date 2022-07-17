# Lab 5: Implement `malloc` and `free`

# <span style="color: red">Before releasing the lab, please remove all ***mymalloc\*.c*** files in the repo!</span>

# 0. Introduction

In this lab you will build a malloc implementation from scratch. We will start from building a most simple allocator, the mmap allocator, which simply indirects all the alloc and free requests to `mmap` and `munmap` calls. Based on the mmap allocator, we will then build a simple single freelist allocator that manages the memory ourselves.

The header file _mymalloc.h_ in the repo provides the signature of the malloc and free functions. Please note here we renamed them to `my_malloc` and `my_free`, because extra difficulties would be involved if debugging a library with overrided `malloc` and `free` implementation.

Just like `malloc` / `free`,
  * `void* my_malloc(size_t size)` is called when the user program wants to allocate some memory. The argument is the size (in bytes) of the allocation. The function should return a pointer pointing to the start of the allocated memory. The result start address should be **word aligned (8 bytes on 64-bit machines)**.
  * `void free(void* ptr)` is for releasing a previously allocated memory. The argument is the start address of the allocated memory.

In this lab, you will be required to provide your own implementation to these two functions. We provide a set of tests with a testing script to check the correctness of your implementation.

For your development, please use a linux distribution or macOS with `clang` installed.

**Note:** Please create an empty `mymalloc.c` file in this repo. We will implement the allocators totally in this file.

# 1. A mmap-based allocator

This mmap-based allocator simply indirects the `my_malloc` and `my_free` calls to `mmap` / `munmap`.

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

For this mmap allocator, if `my_malloc` is called to allocate $N$ bytes, we directly call `mmap` to allocate a chunk with size _no less than_ $N$ bytes. Each `mmap`-ed chunk has a piece of metadata memory for us to record the size of the chunk. So later when we call `munmap`, we will always know the size of the `mmap`-ed chunk.

We first define the `Chunk` data structure, and set an appropriate max allocation size we're going to support, say, 16MB.

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


## Metadata reservation and mmap size

Note that (1) The chunk metadata requires extra space in addition to the allocation size. (2) The size passed to `mmap` must be a multiple of page size (4096 bytes). Our `my_malloc` implementation is not fully correct yet.

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

* For alloaction with zero size or greater than the `max_allocation_size()`, `my_malloc` should return `NULL`.
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


## Putting all together

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

