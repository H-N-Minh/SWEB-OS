#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "pthread.h"

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
    pid_t cid = fork();
    if (cid == 0)
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

        printf("\n\nUserspace locking tests finished\n");

        return 0;
    }
    else
    {
        int status;
        waitpid(cid, &status, 0);
        sleep(2);
        
        int num = get_thread_count();
        if (num == 7 || num == 6)
        {
            printf("All threads are destroyed correctly\n");
            return 0;
        }
        else
        {
            printf("%d threads are still alive\n", num);
            return -1;
        }
        
    }
    
}
