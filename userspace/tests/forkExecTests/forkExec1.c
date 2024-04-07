#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"


// Test a very basic fork
int forkExec1()
{
    pid_t pid = fork();

    if (pid < 0) 
    {
        return -1;
    } 
    else if (pid == 0)
    {
        //const char * path = "usr/pthreadCancelTests.sweb";
        //const char * path = "usr/pthreadCreateTests.sweb";
        const char * path = "usr/pthreadCreateTests.sweb";
        char *argv[] = { (char *)0 };

        int rv = execv(path, argv);
        assert(rv);
        assert(0);
    } 
    else
    {

    pid = fork();

    if (pid < 0) 
    {
        return -1;
    } 
    else if (pid == 0)
    {
        //const char * path = "usr/pthreadCancelTests.sweb";
        //const char * path = "usr/pthreadCreateTests.sweb";
        const char * path = "usr/pthreadCreateTests.sweb";
        char *argv[] = { (char *)0 };

        int rv = execv(path, argv);
        assert(rv);
        assert(0);
    } 
    else
    {
        printf("parent sucessful\n");
        //printf("Hello from parent process!\n");
    }



        printf("parent sucessful\n");
        //printf("Hello from parent process!\n");
    }

    return 0;
}