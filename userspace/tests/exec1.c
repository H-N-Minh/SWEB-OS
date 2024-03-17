#include <assert.h>
#include "unistd.h"


//Exec without arguments
int main()
{
    const char * path = "usr/exec1_testprogram.sweb";

    int rv = execv(path, NULL);

    assert(rv);


    assert(0);    //this should never be reached
}