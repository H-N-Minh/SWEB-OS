#include "stdio.h"
#include "assert.h"

//testprogram for exec3 with args 
int main(int argc, char *argv[])
{
    printf("Argc is %d\n", argc);

    for(int i = 0; i < argc;  i++)
    {
        printf("Argument %d is \"%s\"\n", i,  argv[i]);
    }


    printf("\nExec3 successfull!\n");
    return 0;
}