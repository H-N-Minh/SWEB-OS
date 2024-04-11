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



int main()
{
    // spin1();   //checks if basic locking and unlocking works with spinlock
    // spin2();   //simple syncronization with spinlock 
    // spin3();   //syncronization with two spinlocks
    // spin4();   //Testing more posix errorchecks details

    // mutex1();     //checks if basic locking and unlocking works with mutexes
    mutex2();     //simple syncronization with mutexes
    // mutex3();     //syncronization with two mutexes
    // mutex4();     //Testing more posix errorchecks details

    //Testing more threads to wait on same lock at same time. 
    // Also test if threads holding lock is killed when program exist. NOTE: 100 of threads still run after this test (this is intended)
    // mutex5();     

    printf("\n\nUserspace locking tests successful, press f12 to check if any threads are still running\n");
    return 0;
}
