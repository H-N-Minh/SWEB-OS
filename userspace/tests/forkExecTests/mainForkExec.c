#include "stdio.h"

extern int fe1();
extern int fe2();
extern int forkExec1();
extern int forkExec2();


int main()
{
    fe1();
    sleep(15);
    fe2();
    sleep(15);

    //forkExec1();     //fork with exec
    // forkExec2();     //many forks with many execs


}