#include "testing.h"

int main()
{
    int *mem1 = (int *)mallocing(8);
    int *mem2 = (int *)mallocing(8);
    int *mem3 = (int *)mallocing(8);
    freeing(mem2);
    return *mem1 + *mem3;
}
