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
  printf("test pipe\n");
    int file_descriptor_array[2];
  printf("test pipe\n");
    pipe(file_descriptor_array);printf("test pipe\n");
  printf("test pipe\n");
    int read_fd = file_descriptor_array[0];
  printf("test pipe\n");
    int write_fd = file_descriptor_array[1];

    pthread_t writer, reader;
    pthread_create(&writer, NULL, write_to_pipe, &write_fd);
    pthread_create(&reader, NULL, read_from_pipe, &read_fd);

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    return 0;
}