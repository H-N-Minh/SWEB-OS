#include "assert.h"
#include "unistd.h"
#include "stdio.h"


int exec2()
{
    //Execv with invalid path

    const char * path = "usr/invalid.sweb";

    int rv = execv(path, NULL);
    assert(rv == -1 && "Exec should fail for invalid path");

    printf("Exec2 successfull!\n");
    return 0;
}