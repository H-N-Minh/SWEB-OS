#include "stdio.h"

extern int spin1();
extern int spin2();
extern int spin3();
extern int spin4();

extern int mutex1();
extern int mutex2();
extern int mutex3();
extern int mutex4();
extern int mutex5();

extern int mixLocking();



int main()
{
    spin1();   //checks if basic locking and unlocking works with spinlock
    spin2();   //simple syncronization with spinlock 
    spin3();   //syncronization with two spinlocks
    spin4();   //Testing more posix errorchecks details

    mutex1();     //checks if basic locking and unlocking works with mutexes
    mutex2();     //simple syncronization with mutexes
    mutex3();     //syncronization with two mutexes
    mutex4();     //Testing more posix errorchecks details
    mutex5();     //Test multiple threads waiting on same lock, also test if they are killed when main exits. Check file for more detail

    mixLocking(); ////Test many locking mechanism in 1 file

    printf("\n\nUserspace locking tests successful, press f12 to check if any threads are still running\n");
    return 0;
}
