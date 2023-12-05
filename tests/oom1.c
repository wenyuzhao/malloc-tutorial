#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "testing.h"
#include "../mymalloc.h"

#define SIZE 50000

int main()
{
    set_mem_limit((16ull << 20) << 4 /* bytes */);

    volatile int **ptr_arr = my_malloc(sizeof(int *) * SIZE);

    for (int i = 0; i < SIZE; i++) {
        volatile int *ptr = my_malloc(sizeof(int) * 1024);
        CHECK_NULL(ptr);
        ptr[i % 1024] = i;
        ptr_arr[i % 1024] = ptr;
    }

    for (int i = 0; i < SIZE; i++) {
        my_free((void *) ptr_arr[i]);
    }

    my_free(ptr_arr);

    return EXIT_SUCCESS;
}
