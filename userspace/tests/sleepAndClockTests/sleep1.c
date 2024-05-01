#include "stdio.h"
#include "assert.h"



int sleep1()
{
    printf("Sleeping for 2 seconds start.\n");
    int rv = sleep(2);
    printf("Sleeping for 2 seconds ends.\n");
    assert(rv == 0 && "Successful sleep should return 0.");

    printf("Sleeping for 5 seconds start.\n");
    rv = sleep(5);
    printf("Sleeping for 5 seconds ends.\n");
    assert(rv == 0 && "Successful sleep should return 0.");
    return 0;
}
