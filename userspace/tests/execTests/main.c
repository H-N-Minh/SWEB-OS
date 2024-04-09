#include "stdio.h"

extern int exec1();
extern int exec2();

int main()
{
    exec1();  //Exec without NULL as argument
    exec2();  //exec with wrong path   


    printf("\n\nExec testcases successful\n");

}

                            

                      