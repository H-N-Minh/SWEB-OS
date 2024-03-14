#include <assert.h>
#include "unistd.h"


int main()
{
    const char * path = "usr/exec1_testprogram.sweb";

    int rv = execv(path, NULL);


    assert(0);    //this should never be reached
}