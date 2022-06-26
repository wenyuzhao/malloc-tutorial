#include "testing.h"

#define NALLOCS 200

int main()
{
    void *ptrs[NALLOCS];
    mallocing_loop(ptrs, 16384, NALLOCS);
}
