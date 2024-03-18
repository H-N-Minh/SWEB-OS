#include <assert.h>
#include "unistd.h"


//Exec without arguments
int main()
{
    const char * path = "usr/exec1_testprogram.sweb";

    int rv = execv(path, NULL);

    assert(rv == -1);            //not sure if this is right TODO


    //assert(0);    //this should never be reached
}