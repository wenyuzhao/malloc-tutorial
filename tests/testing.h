#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../mymalloc.h"

static inline void **mallocing_loop(void **array, size_t size, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        void *v = my_malloc(size);
        if (array != NULL)
            array[i] = v;
    }
    return array;
}

static inline void *mallocing(size_t size)
{
    return my_malloc(size);
}

static inline void freeing_loop(void **array, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        my_free(array[i]);
    }
}

static inline void freeing(void *ptr)
{
    my_free(ptr);
}
