#include "stdio.h"

extern int exec1();
extern int exec2();
extern int exec3();
extern int exec4();
extern int exec5();
extern int exec6();
extern int exec7();
extern int exec8();


int main()
{
    exec1();  //Exec without NULL as argument
    exec2();  //exec with wrong path   
    exec3();  //Exec with many arguments
    exec4();  //Exec with arguments
    exec5();  //Exec without arguments
    exec6();  //Exec with multiple threads
    exec7();  //Exec with multiple threads
    // exec8();  //Exec with two pages of arguments (Todo)
          

    sleep(3);
    printf("\n\nExec testcases successful\n");

}

                            

                      