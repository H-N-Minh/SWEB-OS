#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#define TEST_FILE "usr/test1.txt"
#define MSG_SIZE 100
#define BUF_SIZE 128

void* write_to_pipe(void* arg) {
  int write_fd = *((int*)arg);
  char msg[MSG_SIZE] = "Testing pipe communication!";
  write(write_fd, msg, sizeof(msg));
  return NULL;
}

void* read_from_pipe(void* arg) {
  int read_fd = *((int*)arg);
  char buf[MSG_SIZE] = {0};
  read(read_fd, buf, sizeof(buf));
  printf("Read from pipe: %s\n", buf);
  return NULL;
}

void pipeTest() {
  printf("Starting pipe test...\n");
  int pipefd[2];
  pipe(pipefd);

  pthread_t writer, reader;
  pthread_create(&writer, NULL, write_to_pipe, &pipefd[1]);
  pthread_create(&reader, NULL, read_from_pipe, &pipefd[0]);

  pthread_join(writer, NULL);
  pthread_join(reader, NULL);

  close(pipefd[0]);
  close(pipefd[1]);
  printf("Pipe test completed.\n");
}


void fdTest() {
  printf("Starting fd test...\n");
  char buf[BUF_SIZE];
  int local_fd;
  ssize_t bytesRead, bytesWritten;

  printf("Opening file: %s\n", TEST_FILE);
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    assert(0);
    return;
  }

  printf("Opening all files between 'usr/file0.txt' and 'usr/file4.txt'\n");
  int fds[5];
  char path[20] = "usr/file0.txt";
  for(int i = 0; i < 5; i++) {
    path[8] = '0' + i;
    fds[i] = open(path, O_RDWR);
    printf("Opened file: %s --> Local FD: %d\n", path, fds[i]);
  }

  printf("Writing to file: %s\n", TEST_FILE);
  char* writeData = ":/";
  bytesWritten = write(local_fd, writeData, strlen(writeData));
  if (bytesWritten < 0) {
    printf("Failed to write to file\n");
    assert(0);
    return;
  }

  printf("Reading from file: %s\n", TEST_FILE);
  lseek(local_fd, 0, SEEK_SET);
  bytesRead = 0;
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0) {
    printf("Failed to read from file\n");
    assert(0);
    return;
  }

  buf[bytesRead] = '\0';
  printf("Read from file: %s\n", buf);

  printf("Closing file: %s\n", TEST_FILE);
  if (close(local_fd) < 0) {
    printf("Failed to close file\n");
    assert(0);
    return;
  }

  for (int i = 0; i < 5; i++) {
    close(fds[i]);
  }

  printf("Fd test completed.\n");
}

void* pipeTestMain(void* arg) {
  printf("\n ==== Running pipe tests while operating on file descriptors! ====\n");

  fdTest();

  pipeTest();

  return NULL;
}

void* fdTestMain(void* arg) {
  printf("\n ==== Running file descriptor tests while operating on pipes! ====\n");

  pipeTest();

  fdTest();

  return NULL;
}

int main() {
  pthread_t thread1, thread2;

  if(pthread_create(&thread1, NULL, pipeTestMain, NULL)) {
    printf("Error creating thread\n");
    return 1;
  }

  if(pthread_create(&thread2, NULL, fdTestMain, NULL)) {
    printf("Error creating thread\n");
    return 1;
  }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  printf("All tests completed.\n");
  return 0;
}