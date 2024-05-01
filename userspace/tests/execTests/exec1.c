#include "assert.h"
#include "unistd.h"


//Exec without NULL as argument
int exec1()
{
    const char * path = "usr/exec_testprogram.sweb";
    int rv = execv(path, NULL);

    assert(rv == -1);
    printf("Exec1 successful!\n");
    return 0;
}