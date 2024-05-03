#include "unistd.h"
#include <stdio.h>
#include <pthread.h>

#define MSG_SIZE 100

void *write_to_pipe(void *arg) {
  int write_fd = *((int *)arg);
  char msg[MSG_SIZE] = "Testing pipe communication!";
  write(write_fd, msg, sizeof(msg));
  return NULL;
}

void *dup_and_read_from_pipe(void *arg) {
  int read_fd = dup(*((int *)arg));
  char buf[MSG_SIZE] = {0};
  read(read_fd, buf, sizeof(buf));
  printf("Read from pipe: %s\n", buf);
  close(read_fd);
  return NULL;
}

void *dup_invalid_fd(void *arg) {
  int invalid_fd = dup(1000);
  if (invalid_fd == -1) {
    printf("Tried to duplicate an invalid file descriptor, operation failed as expected\n");
  } else {
    printf("Unexpected behavior! Managed to duplicate an invalid file descriptor\n");
  }
  return NULL;
}

int main() {
  printf("Test started\n");
  int file_descriptor_array[2];
  pipe(file_descriptor_array);

  int original_read_fd = file_descriptor_array[0];
  int write_fd = file_descriptor_array[1];

  pthread_t writer, reader, dup_tester;
  pthread_create(&writer, NULL, write_to_pipe, &write_fd);
  pthread_create(&reader, NULL, dup_and_read_from_pipe, &original_read_fd);
  pthread_create(&dup_tester, NULL, dup_invalid_fd, NULL);

  pthread_join(writer, NULL);
  pthread_join(reader, NULL);
  pthread_join(dup_tester, NULL);

  printf("Test finished\n");
  return 0;
}