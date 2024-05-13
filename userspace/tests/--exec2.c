/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


#include "assert.h"
#include "unistd.h"
#include "stdio.h"


int main()
{
    //Execv with invalid path

    const char * path = "usr/invalid.sweb";
    char *argv[] = { "Alle meine", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    int rv = execv(path, argv);
    assert(rv == -1 && "Exec should fail for invalid path");
    printf("Exec2 successful!\n");
    return 0;
}