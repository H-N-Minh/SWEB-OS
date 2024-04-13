#include "stdio.h"

extern int pd1();
extern int pd2();
extern int pd3();

int main()
{
    pd1(); 
    pd2();
    pd3();

    printf("\n\npthread_detach testcases successful\n");
}
