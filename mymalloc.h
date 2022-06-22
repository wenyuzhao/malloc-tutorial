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

void *my_malloc(size_t size);
// void *my_calloc(size_t nmemb, size_t size);
// void *my_realloc(void *ptr, size_t size);
void my_free(void *p);
