#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"
#include "time.h"


// Clock in two processes
int clock2()
{
  pid_t pid = fork();
  assert(pid >= 0);
  if (pid == 0) 
  {
    unsigned int clock_time1  = clock();
    sleep(2);
    unsigned int clock_time2  = clock();
    printf("clock time 1: %u\n", clock_time1);
    printf("clock time 2: %u\n", clock_time2);
    float difference_between_messurements = (float)(clock_time2 - clock_time1)/CLOCKS_PER_SEC;
    printf("Difference for clock with sleep: %f. (Should be around zero)\n",  difference_between_messurements);
    assert((int)difference_between_messurements == 0);
  }
  else 
  {
    unsigned int clock_time1  = clock();
    sleep(2);
    unsigned int clock_time2  = clock();
    printf("clock time 1: %u\n", clock_time1);
    printf("clock time 2: %u\n", clock_time2);
    float difference_between_messurements = (float)(clock_time2 - clock_time1)/CLOCKS_PER_SEC;
    printf("Difference for clock with sleep: %f. (Should be around zero)\n",  difference_between_messurements);
    assert((int)difference_between_messurements == 0);
    exit(0);
  }
  return 0;
}