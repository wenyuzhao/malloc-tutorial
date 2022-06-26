#include "testing.h"

int allocations = 100;

int main()
{
    char *ptrs[15410];
    int i;
    for (i = 0; i < allocations; i++)
    {
        char *p1 = (char *)mallocing(100);
        ptrs[i] = p1;
    }
    for (i = 0; i < allocations; i = i + 2)
    {
        freeing(ptrs[i]);
    }
}
