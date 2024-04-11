#include "stdio.h"

extern int forkExec1();
extern int forkExec2();


int main()
{
    //forkExec1();     //fork with exec
    forkExec2();     //many forks with many execs


}