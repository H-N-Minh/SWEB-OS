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


int pc8()
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


  //testing set and test for joinable (should now be detach or joinable)
  rv = pthread_attr_init(&attr_3);
  assert(rv == 0);

  rv = pthread_attr_setdetachstate(&attr_3, PTHREAD_CREATE_JOINABLE);
  assert(rv == 0);
  rv = pthread_attr_getdetachstate(&attr_3, &detach_state);
  assert(rv == 0);
  assert(detach_state == PTHREAD_CREATE_JOINABLE);

  rv = pthread_create(&thread_id_3, &attr_3, (void*)function_psd1_2, NULL);
  assert(rv == 0);
  rv = pthread_detach(thread_id_3);
  assert(rv == 0);




  
  
  
  //int pthread_attr_init(pthread_attr_t *attr);
    // pthread_t thread_id;
    // int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function_psd1, NULL);
    // assert(pthread_create_rv == 0);

    // sleep(1);

    // void* value_ptr;
    // int pthread_join_rv = pthread_join(thread_id, &value_ptr);
    // assert(pthread_join_rv == 0);
    // assert((size_t)value_ptr == 6 && "value_ptr does not match the returnvalue of thread");

    return 0;
}