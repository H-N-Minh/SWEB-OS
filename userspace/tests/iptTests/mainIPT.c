#include "stdio.h"

extern int ipt1();
extern int ipt2();

int main()
{
    ipt1();
    printf("first done\n");
    ipt2();
    printf("second done\n");


    printf("\n\nipt testcases successful\n");
}