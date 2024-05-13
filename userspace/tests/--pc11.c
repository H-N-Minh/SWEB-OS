/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int function_psd1()
{
    return 6;
}

int function_psd1_2()
{
    while(1){}
    return 4;
}


int main()
{
  //pthread create with wrong attributes fails (we only have check fo detach)
  pthread_t thread_id;
  pthread_attr_t attr = {99,(void*)99,99,99};
  int rv = pthread_create(&thread_id, (void*)&attr, (void*)function_psd1, NULL);
  assert(rv != 0);

  //using pthread attributes twice for different threads works
  pthread_attr_t attr2;
  rv = pthread_attr_init(&attr2);
  assert(rv == 0);
  rv = pthread_create(&thread_id, (void*)&attr2, (void*)function_psd1, NULL);
  assert(rv == 0);
  rv = pthread_create(&thread_id, (void*)&attr2, (void*)function_psd1, NULL);
  assert(rv == 0);

  //destroying twice should fail
  rv = pthread_attr_destroy(&attr2);
  assert(rv == 0);

  rv = pthread_attr_destroy(&attr2);
  assert(rv != 0);


  //using a destroyed attribute should fail
  rv = pthread_create(&thread_id, (void*)&attr2, (void*)function_psd1, NULL);
  assert(rv != 0);

  //destroy attribute with address not in userspace should fail
  rv = pthread_attr_destroy((void*)0x0000800000000000ULL);
  assert(rv != 0);

  //init attribute with address not in userspace should fail
  rv = pthread_attr_init((void*)0x0000800000000000ULL);
  assert(rv != 0);


  pthread_t thread_id_3;
  pthread_attr_t attr_3;

  rv = pthread_attr_init(&attr_3);
  assert(rv == 0);

  //detachstate after init should be joinable
  int detach_state;
  rv = pthread_attr_getdetachstate(&attr_3, &detach_state);
  assert(rv == 0);
  assert(detach_state == PTHREAD_CREATE_JOINABLE);

  //setting pthread_attribute to detach
  rv = pthread_attr_setdetachstate(&attr_3, PTHREAD_CREATE_DETACHED);
  assert(rv == 0);

  //detachstate should be now detached
  rv = pthread_attr_getdetachstate(&attr_3, &detach_state);
  assert(rv == 0);
  assert(detach_state == PTHREAD_CREATE_DETACHED);

  rv = pthread_create(&thread_id_3, &attr_3, (void*)function_psd1_2, NULL);
  assert(rv == 0);

  //join and cancel should fail if state is detached
  rv = pthread_join(thread_id_3, NULL);
  assert(rv != 0);
  rv = pthread_detach(thread_id_3);
  assert(rv != 0);

  //set and get should fail if attributes is not initalized
  rv = pthread_attr_destroy(&attr_3);
  assert(rv == 0);

  rv = pthread_attr_setdetachstate(&attr_3, PTHREAD_CREATE_DETACHED);
  assert(rv != 0);
  rv = pthread_attr_getdetachstate(&attr_3, &detach_state);
  assert(rv != 0);

 return 0;
}


//pthread create with wrong attributes
//using pthread attributes twice for different threads
//destroying twice
//using a destroyed attribute
//destroy attribute with address not in userspace
//init attribute with address not in userspace
//detachstate after init should be joinable
//setting pthread_attribute to detach
//join and cancel if state is detached
//set and get if attributes is not initalized