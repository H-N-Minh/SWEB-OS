#include "assert.h"
#include "unistd.h"


//Exec without arguments
int exec1()
{
    const char * path = "usr/exec1_testprogram.sweb";

    int rv = execv(path, NULL);

    assert(rv == -1);            //not sure if this is right TODOs

    printf("exec 1 successfull\n");
    //assert(0);    //this should never be reached
    return 0;
}