#include <assert.h>
#include "unistd.h"


//Test: function that called exec calls exec
int main()
{
    const char * path = "usr/exec3.sweb";
    char *argv[] = { "Alle meine", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    int rv = execv(path, argv);

    assert(rv);
    
    assert(0);    //this should never be reached
}