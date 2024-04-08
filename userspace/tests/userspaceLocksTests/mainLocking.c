#include "stdio.h"

extern int spin1();
extern int spin2();
extern int spin3();
extern int spin4();


int main()
{
    spin1();   //checks if basic locking and unlocking works with spinlock
    spin2();   //simple syncronization with spinlock 
    spin3();   //syncronization with two spinlocks
    spin4();   //Testing more posix errorchecks details

    printf("\n\nUserspace locking tests successful");
}
