#include "testing.h"

int allocations = 100;

int main()
{
    int i;
    for (i = 0; i < allocations; i++)
    {
        char *p1 = (char *)mallocing(100);
        USE(p1);
    }
    char *p2 = (char *)mallocing(64);
    char *p3 = (char *)mallocing(8);
    USE(p2);
    USE(p3);
}
