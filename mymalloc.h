#include <stddef.h>

#define USE(...)             \
    do                       \
    {                        \
        (void)(__VA_ARGS__); \
    } while (0)

#ifdef ENABLE_LOG
#define LOG(...) fprintf(stderr, "[malloc] " __VA_ARGS__);
#else
#define LOG(...)
#endif

#define N_LISTS 59

extern const size_t kChunkSize;
extern const size_t kBlockMetadataSize;
extern const size_t kFenceSize;

static inline size_t max_allocation_size()
{
    return kChunkSize - kBlockMetadataSize - (kFenceSize << 1);
}

void *my_malloc(size_t size);
void my_free(void *p);
void verify();
