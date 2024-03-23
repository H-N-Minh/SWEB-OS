#include "stdio.h"
#include "assert.h"



int main()
{
    printf("Sleep for one second");
    int rv = sleep(1);
    printf("Slept for one second");
    assert(rv == 0 && "Successful sleep should return 0.");
    return 0;
}
