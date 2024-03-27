#include "stdio.h"
#include "assert.h"



int main()
{
    printf("Sleeping start.\n");
    int rv = sleep(10);
    printf("Sleeping end.\n");
    assert(rv == 0 && "Successful sleep should return 0.");
    return 0;
}
