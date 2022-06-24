# malloc-tutorial

# Run tests:

```
make test MALLOC=mymalloc
```

Where `MALLOC=?` can be one of the following values:
* `mmapmalloc` - redirect memory requests to mmap/munmap calls
* `mymalloc` - DLMalloc with single list, block coalescing, and fence posting

Specify `LOG=1` (`make test MALLOC=mymalloc LOG=1`) will enable logging.

Specify `RELEASE=1` (`make test MALLOC=mymalloc RELEASE=1`) will compile everything `-O3`.

# TODO

- [ ] More tests (maybe https://github.com/ramankahlon/CS252/tree/master/lab1-src/tests/testsrc ?)
- [x] DLMalloc with single list, block coalescing, and fence posting
- [x] Optimization: Constant time coalescing
- [ ] Optimization: Segregated Free List
- [ ] Optimization: Reduce metadata footprint
- [ ] Optimization: Coalescing chunks from the OS