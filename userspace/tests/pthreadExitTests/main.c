#include "stdio.h"

extern int pe1();
extern int pe2();

int main()
{
    pe1();    //Simple pthread_create with pthread exit in function
    pe2();    //similar to pe1

    printf("\n\npthread_exit testcases successful\n");
}
    