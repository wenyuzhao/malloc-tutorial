# malloc-tutorial

# Run tests:

```
make test MALLOC=mymalloc
```

Where `MALLOC=?` can be one of the following values:
* `mmapmalloc` - redirect memory requests to mmap/munmap calls
* `mymalloc`   - DLMalloc with single list, coalescing, and fenceposts
* `mymalloc2`  - DLMalloc with single list, _constant-time_ coalescing, and fenceposts
* `mymalloc3`  - DLMalloc with **segregated** lists, **constant-time** coalescing, and fenceposts
* `mymalloc4`  - DLMalloc with **segregated** lists, **constant-time** coalescing, fenceposts, and **metadata footprint reduction**
* `mymalloc5`  - DLMalloc with **segregated** lists, **constant-time** coalescing, fenceposts, **metadata footprint reduction**, and **chunk coalescing**

Specify `LOG=1` (`make test MALLOC=mymalloc LOG=1`) will enable logging.

Specify `RELEASE=1` (`make test MALLOC=mymalloc RELEASE=1`) will compile everything `-O3`.

# TODO

- [x] More tests (maybe https://github.com/ramankahlon/CS252/tree/master/lab1-src/tests/testsrc ?)
- [x] DLMalloc with single list, block coalescing, and fence posting
- [x] Optimization: Constant time coalescing
- [x] Optimization: Segregated Free List
- [x] Optimization: Metadata footprint reduction
- [x] Optimization: Coalescing chunks from OS