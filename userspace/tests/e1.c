#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "assert.h"



int main()     //exit in main
{
    printf("e1 successful if no assertion gets raised!\n");
    exit(EXIT_SUCCESS);
    assert(0);
}
