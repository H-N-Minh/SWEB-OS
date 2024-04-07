#include "assert.h"
#include "unistd.h"


//Test: Exec without arguments
int main()
{
    const char * path = "usr/exec5_testprogram.sweb";
    char *argv[] = { (char *)0 };

    int rv = execv(path, argv);
    assert(0);

    return 0;
}