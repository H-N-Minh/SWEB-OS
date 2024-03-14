#include <assert.h>
#include "unistd.h"


int main()
{
    const char * path = "usr/shell.sweb";   //not sure if path is right -- pretty sure it is not

    int rv = execv(path, NULL);


    assert(0);    //this should never be reached
}