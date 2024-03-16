#include <assert.h>
#include "unistd.h"


//Test: exec with arguments
int main()
{
    const char * path = "usr/exec1_testprogram.sweb";

    int rv = execv(path, NULL);

    assert(rv);


    assert(0);    //this should never be reached
}