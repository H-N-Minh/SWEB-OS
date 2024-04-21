    
    
    
    
void pc7()
{
  //Test7: Invalid userspace address as thread_id
  rv = pthread_create((void*)0x7ffffffa0000, NULL, (void * (*)(void *))function1, NULL);  //todos: move somewhere else
  assert(0);
}

