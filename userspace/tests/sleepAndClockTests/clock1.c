#include "stdio.h"
#include "time.h"

int clock1()
{
    for(int i = 0; i < 10000; i++)
    {
      printf("i %d\n", i);
    }
    unsigned int clock_time  = clock();
    printf("Clock returned %u.\n", clock_time);
    printf("Clock in seconds returned %f.\n", (float)clock_time/CLOCKS_PER_SEC);
    return 0;
}
