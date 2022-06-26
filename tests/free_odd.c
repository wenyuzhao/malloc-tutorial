#include "testing.h"

#define NALLOCS 10

int main()
{
    void *ptrs[NALLOCS];

    mallocing_loop(ptrs, 8, NALLOCS);
    for (int i = NALLOCS - 1; i > 0; i -= 2)
    {
        freeing(ptrs[i]);
    }
}
