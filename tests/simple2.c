#include "testing.h"

int main()
{
    int *mem1 = (int *)mallocing(8);
    *mem1 = 10;
    int *mem2 = (int *)mallocing(8);
    int *mem3 = (int *)mallocing(8);
    return *mem1 - *mem1 + *mem2 + *mem3;
}
