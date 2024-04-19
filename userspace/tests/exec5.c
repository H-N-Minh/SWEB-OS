#include "assert.h"
#include "unistd.h"


//Test: Exec without arguments
int main()
{
    printf("hey\n");
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { (char *)0 };

    execv(path, argv);
    assert(0);

    return 0;
}

