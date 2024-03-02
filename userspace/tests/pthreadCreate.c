#include "pthread.h"
#include "stdio.h"

void hello() {
  printf("hii this is the thread func\n");
}

int main() {
  pthread_t thread;
  pthread_create(&thread, NULL, (void*) &hello, NULL);
  return 0;
}