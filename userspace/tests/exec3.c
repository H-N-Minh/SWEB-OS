#include "assert.h"
#include "unistd.h"


//Test: exec with arguments
int main()
{
    const char * path = "usr/exec3_testprogram.sweb";
    char *argv[] = { "Alle meine", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    int rv = execv(path, argv);

    assert(rv);
    
    assert(0);    //this should never be reached
}