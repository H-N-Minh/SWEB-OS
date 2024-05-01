
#include "stdio.h"
#include "time.h"
#include "pthread.h"
#include "assert.h"

int ready = 0;
int flag = 0;


void sleep2seconds()
{
  ready++;
  while(ready != 2){}
  sleep(2);
  flag = 1;
}

void clock_without_sleep()
{
  ready++;
  while(ready != 2){}
  unsigned int clock_time1  = clock();
  while(!flag){}
  unsigned int clock_time2  = clock();
  printf("clock time 1: %u\n", clock_time1);
  printf("clock time 2: %u\n", clock_time2);
  float difference_between_messurements = (float)(clock_time2 - clock_time1)/CLOCKS_PER_SEC;
  printf("Difference for clock with sleep: %f. (Should be around 2)\n",  difference_between_messurements);
  assert((int)difference_between_messurements == 2);
}

void clock_with_sleep()
{
  unsigned int clock_time1  = clock();
  sleep(2);
  unsigned int clock_time2  = clock();
  printf("clock time 1: %u\n", clock_time1);
  printf("clock time 2: %u\n", clock_time2);
  float difference_between_messurements = (float)(clock_time2 - clock_time1)/CLOCKS_PER_SEC;
  printf("Difference for clock with sleep: %f. (Should be close to zero)\n", difference_between_messurements);
  assert((int)difference_between_messurements == 0);
}

int clock1()
{
  pthread_t thread_id1;
  pthread_t thread_id2;

  pthread_create(&thread_id1, NULL, (void*)clock_without_sleep, NULL);
  pthread_create(&thread_id2, NULL, (void*)sleep2seconds, NULL);

  pthread_join(thread_id1, NULL);
  pthread_join(thread_id2, NULL);

 pthread_t thread_id3;
  pthread_create(&thread_id3, NULL, (void*)clock_with_sleep, NULL);
  pthread_join(thread_id3, NULL);

  return 0;
}

