#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>


#define TEST_FILE "usr/file0.txt"
#define BUF_SIZE 128

void* read_from_pipe(void* arg) {
  int read_fd = *((int*)arg);
  char buf[BUF_SIZE] = {0};
  read(read_fd, buf, sizeof(buf));
  printf("Read from pipe: %s\n", buf);
  return NULL;
}

int main() {
  printf("Starting file descriptor write and pipe read test...\n");
  int pipefd[2];
  pipe(pipefd);

  pthread_t reader;
  pthread_create(&reader, NULL, read_from_pipe, &pipefd[0]);

  int fd = open(TEST_FILE, O_WRONLY);

  char* message = "Testing fd write and pipe read!";
  write(fd, message, strlen(message));
  close(fd);

  fd = open(TEST_FILE, O_RDONLY);

  char buf[BUF_SIZE];
  read(fd, buf, BUF_SIZE - 1);
  write(pipefd[1], buf, BUF_SIZE);

  pthread_join(reader, NULL);

  close(pipefd[0]);
  close(pipefd[1]);
  printf("FD write and pipe read test completed.\n");
  return 0;
}
