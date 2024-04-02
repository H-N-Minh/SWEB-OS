#include "unistd.h"
#include <stdio.h>
#include <pthread.h>

#define MSG_SIZE 100

void *write_to_pipe(void *arg) {
    int write_fd = *((int *)arg);
    char msg[MSG_SIZE] = "Testing pipe IPC";
    write(write_fd, msg, sizeof(msg));
    return NULL;
}

void *read_from_pipe(void *arg) {
    int read_fd = *((int *)arg);
    char buf[MSG_SIZE] = {0};
    read(read_fd, buf, sizeof(buf));
    printf("Read from pipe: %s\n", buf);
    return NULL;
}

int main(void) {
  printf("test pipe1\n");
    int file_descriptor_array[2];
  printf("test pipe2\n");
    pipe(file_descriptor_array);printf("test pipe\n");
  printf("test pip3e\n");
    int read_fd = file_descriptor_array[0];
  printf("test pipe4\n");
    int write_fd = file_descriptor_array[1];
  printf("test pipe5\n");
    pthread_t writer, reader;
    pthread_create(&writer, NULL, write_to_pipe, &write_fd);
  printf("test pipe6\n");
    pthread_create(&reader, NULL, read_from_pipe, &read_fd);
  printf("test pipe7\n");

    pthread_join(writer, NULL);
  printf("test pipe8\n");
    pthread_join(reader, NULL);
  printf("test ende\n");
    return 0;
}