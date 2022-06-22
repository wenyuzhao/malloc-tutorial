#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_LOG
  #define LOG(...) fprintf(stderr, "[malloc] " __VA_ARGS__);
#else
  #define LOG(...)
#endif

#define USE(...) do { (void)(__VA_ARGS__); } while (0)
