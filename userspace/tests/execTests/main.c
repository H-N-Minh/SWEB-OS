#include "stdio.h"

extern int exec1();
extern int exec2();
extern int exec3();

int main()
{
    exec1();  //Exec without NULL as argument
    exec2();  //exec with wrong path   
    exec3();  //Exec with too many arguments (Todos, look if that actually should fail)

    //exec4 //exec with arguments
    //exec5 //exec without arguments

    printf("\n\nexec testcases successful\n");

}

                            

                      