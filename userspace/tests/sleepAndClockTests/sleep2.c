#include "stdio.h"
#include "assert.h"
#include "pthread.h"


int sleepfunction()
{
    printf("Sleeping for 2 seconds start.\n");
    int rv = sleep(2);
    printf("Sleeping for 2 seconds ends.\n");
    assert(rv == 0 && "Successful sleep should return 0.");

    return 0;
}

int sleep2()
{
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, (void*)sleepfunction, NULL);
  pthread_join(thread_id, NULL);
  return 0;
}
