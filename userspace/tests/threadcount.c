#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* threadFunction(void* arg) {
  printf("Hello from the new thread!\n");
  return NULL;
}

int main() {
  pthread_t thread_id;

  int result = pthread_create(&thread_id, NULL, threadFunction, NULL);
  if (result != 0) {
    printf("Failed to create the thread. Error code: %d\n", result);
    return -1;
  }
  printf("Thread created successfully.\n");

  result = pthread_join(thread_id, NULL);
  if (result != 0) {
    printf("Failed to join the thread. Error code: %d\n", result);
    return -1;
  }

  return 0;
}
