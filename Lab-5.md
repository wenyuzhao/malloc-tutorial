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

We will now use a global implicit freelist to manage the memory on our own.

Same as before, we will start from [an empty implementation file](#getting-started) and name it as _mymalloc.c_.

## Acquire initial memory

We're going to acquire a single piece of memory (64 MB) once from OS, and manage it on our own. This is enough for all of our current tests. Note that in the assignment, you will be asked to acquire more memory from OS incrementally, so that programs can increase their memory usage as required.

```c
// mymalloc.c
#include <assert.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include "mymalloc.h"

const size_t kChunkSize = 16ull << 20; // 16MB mmap chunk
const size_t kMemorySize = kChunkSize << 2; // 64MB mmap chunk
const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize;

static Block *start = NULL;

/// Acquire more memory from OS
static Block *acquire_memory() {
  // Acquire one more chunk from OS
  Block *block = mmap(NULL, kMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  // Initialize block metadata
  block->free = true;
  block->size = kMemorySize;
  return block;
}

void *my_malloc(size_t size) {
  // Initial allocation?
  if (start == NULL) {
    start = acquire_memory();
  }
  return NULL; // Unimplemented
}

void my_free(void *ptr) {
  // Unimplemented
}
```

## Allocate memory from the global freelist

We use a global _implicit_ freelist to manage the memory blocks. Instead of adding explicit next/prev pointers to block metadata, we use block start address and block size to traverse blocks inside a contiguous piece of memory, and treat this as a "implicit" freelist.

The only block metadata we need to maintain is the free/used flag and the block size. For allocation, we simply traverse the list, find the first _free_ block that is large enough, and split it into two blocks if necessary.

```diff
  #include "mymalloc.h"

+ typedef struct Block {
+   bool free;
+   size_t size;
+ } Block;
+
+ const size_t kBlockMetadataSize = sizeof(Block);
  const size_t kChunkSize = 16ull << 20; // 16MB mmap chunk
  const size_t kMemorySize = kChunkSize << 2; // 64MB mmap chunk
  const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize;
+
+ static const size_t kMinAllocationSize = 1;

  static Block *start = NULL;

+ /// Get right neighbour
+ inline static Block *get_right_block(Block *block) {
+   size_t ptr = ((size_t) block) + block->size;
+   if (ptr >= ((size_t) start) + kMemorySize) return NULL;
+   return (Block*) ptr;
+ }
+
+ /// Get data pointer of a block
+ inline static void *block_to_data(Block *block) {
+   return (void *)(((size_t)block) + kBlockMetadataSize);
+ }
+
  /// Acquire more memory from OS
  static Block *acquire_memory() {
    // Acquire one more chunk from OS
    Block *block = mmap(NULL, kMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    // Initialize block metadata
    block->free = true;
    block->size = kMemorySize;
    return block;
  }

+ /// Split a block into two. Both of the blocks will be set as non-free
+ static Block *split(Block *block, size_t size) {
+   assert(block->free);
+   size_t total_size = block->size;
+   Block *first = block;
+   first->free = false;
+   first->size = total_size - size - kBlockMetadataSize;
+   Block *second = get_right_block(first);
+   second->size = total_size - first->size;
+   second->free = false;
+   return second;
+ }
+
  void *my_malloc(size_t size) {
+   if (size == 0 || size > kMaxAllocationSize) return NULL;
    // Initial allocation?
    if (start == NULL) {
      start = acquire_memory();
    }
-   return NULL; // Unimplemented
+   // Find a block in the freelist
+   Block *block = NULL;
+   for (Block *b = start; b != NULL; b = get_right_block(b)) {
+     if (b->free && b->size - kBlockMetadataSize >= size) {
+       block = b;
+       break;
+     }
+   }
+   // Split block if the block is too large
+   if (block->size >= size + (kBlockMetadataSize << 1) + kMinAllocationSize) {
+     Block *second = split(block, size);
+     Block *first = block;
+     first->free = true;
+     block = second;
+   }
+   // Mark block as allocated
+   block->free = false;
+   // Zero memory and return
+   void *data = block_to_data(block);
+   memset(data, 0, size);
+   assert(!block->free);
+   return data;
  }

void my_free(void *ptr) {
}
```

## Release memory to the global freelist

To release a memory block, we get the block start address from the given pointer, and mark the block as free.

```diff
  // mymalloc.c
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

  static const size_t kMinAllocationSize = 1;

  static Block *start = NULL;

  /// Get right neighbour
  inline static Block *get_right_block(Block *block) {
    size_t ptr = ((size_t) block) + block->size;
    if (ptr >= ((size_t) start) + kMemorySize) return NULL;
    return (Block*) ptr;
  }

  /// Get data pointer of a block
  inline static void *block_to_data(Block *block) {
    return (void *)(((size_t)block) + kBlockMetadataSize);
  }

+ /// Get the block of the data pointer
+ inline static Block *data_to_block(void *ptr) {
+   return (Block *)(((size_t)ptr) - kBlockMetadataSize);
+ }
+
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
+   if (ptr == NULL) return;
+   // Get block pointer
+   Block *block = data_to_block(ptr);
+   assert(!block->free);
+   // Mark block as free
+   block->free = true;
  }
```

## Testing and debugging using GDB

If you run all the tests now using `./test.py`, you'll see one test (_align_) failing. Let's use that as an example showing how to use GDB to C programs. Please do this section on Linux instead of macOS.

First let's launch the faulty program using GDB:

```console
$ gdb ./tests/align
GNU gdb (GDB) Fedora 11.2-2.fc35
Copyright (C) 2022 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-redhat-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from ./tests/align...
(gdb)
```

Type `run` to run the program:

```
(gdb) run
Starting program: /home/wenyu/malloc-tutorial/tests/align
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib64/libthread_db.so.1".
align: tests/align.c:12: int main(): Assertion `is_word_aligned(ptr)' failed.

Program received signal SIGABRT, Aborted.
__pthread_kill_implementation (threadid=<optimized out>, signo=signo@entry=6, no_tid=no_tid@entry=0) at pthread_kill.c:44
44            return INTERNAL_SYSCALL_ERROR_P (ret) ? INTERNAL_SYSCALL_ERRNO (ret) : 0;
(gdb)
```

We see that it failed with a message "align: tests/align.c:12: int main(): Assertion 'is_word_aligned(ptr)' failed.". Check _tests/align.c_ line 12, we can see that the first allocation `void *ptr = mallocing(1);` may not return a word-aligned address.

Let's use gdb to dump the value of this `ptr` variable. First, type `backtrace` to display the stack trace:

```console
(gdb) backtrace
#0  __pthread_kill_implementation (threadid=<optimized out>, signo=signo@entry=6, no_tid=no_tid@entry=0) at pthread_kill.c:44
#1  0x00007ffff7e41b53 in __pthread_kill_internal (signo=6, threadid=<optimized out>) at pthread_kill.c:78
#2  0x00007ffff7df52a6 in __GI_raise (sig=sig@entry=6) at ../sysdeps/posix/raise.c:26
#3  0x00007ffff7dc87f3 in __GI_abort () at abort.c:79
#4  0x00007ffff7dc871b in __assert_fail_base (fmt=<optimized out>, assertion=<optimized out>, file=<optimized out>, line=<optimized out>, function=<optimized out>) at assert.c:92
#5  0x00007ffff7dee1f6 in __GI___assert_fail (assertion=0x402010 "is_word_aligned(ptr)", file=0x402025 "tests/align.c", line=12, function=0x402033 "int main()") at assert.c:101
#6  0x00000000004011b7 in main () at tests/align.c:12
```

The stack frame #6 is the main function we're caring about, and also contains the value of `ptr`.

Let's select the frame #6 (by running `select-frame 6`) and dump variables using `info locals`:

```console
(gdb) info locals
ptr = 0x7ffff7d9cfff
ptr2 = 0x9e00000006
```

Since the program crashes at line 12, before `ptr2` is not filled. So the `ptr2` value here is meaningless.

But we can see that the first allocation, `ptr = 0x7ffff7d9cfff`, is not word-aligned. Since this is the first allocation, so it should be relatively easy to debug. Let's use GDB to inspect the implicit free-list. We store the start address of the first block as a global variable `start`. So we simply dump this variable:

```console
(gdb) print start
$1 = (Block *) 0x7ffff3d9d000
(gdb) print *start
$2 = {free = true, size = 67108847}
```

From the above dump we can see that after splitting the 64 MB (67108864 B) block, the first block has size 67108847 B. So the size of the second block should be `67108864 B - 67108847 B = 17 B`, which is exactly the metadata size (16 B) + the allocation request (1 B).

We can also easilty varify that our `split` function is correct: The second block address is `start + first_block_size = 0x7ffff3d9d000 + 67108847 = 0x7ffff7d9cfef`, plus the 16 B metadata size, the value is `0x7ffff7d9cfff`, which is equal to `ptr`.

At this point you may realize that the problem is because that the allocation size is too small (only 1 B). If we make a block only `17 B`, the address of the neighbour blocks cannot be aligned proerly, and eventually results in unaligned allocation result.

So the solution is to round up the allocation size to a multiple of word size. Since the metadata size is also a multiple of word size, we can guarantee that every block in the heap is word-aligned.

```diff
...
  const size_t kMaxAllocationSize = kChunkSize - kBlockMetadataSize;

+ static const size_t kAlignment = sizeof(size_t); // Word alignment
- static const size_t kMinAllocationSize = 1;
+ static const size_t kMinAllocationSize = kAlignment;

...

    return (Block*) ptr;
  }

+ /// Round up size
+ inline static size_t round_up(size_t size, size_t alignment) {
+   const size_t mask = alignment - 1;
+   return (size + mask) & ~mask;
+ }

  /// Get data pointer of a block
  inline static void *block_to_data(Block *block) {

...


  void *my_malloc(size_t size) {
+   // Round up allocation size
+   size = round_up(size, kAlignment);
    if (size == 0 || size > kMaxAllocationSize) return NULL;

```

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
  size = round_up(size, kAlignment);
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
```
