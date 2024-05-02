#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define TEST_FILE "usr/test2.txt"
#define BUF_SIZE  128

//Check if both parent and child can use local file descriptor after fork
int forklfds2() {
  char buf[BUF_SIZE];
  int local_fd;
  ssize_t bytesRead, bytesWritten;
  char* writeData = ":/";

  // printf("Opening file: %s\n", TEST_FILE);
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    assert(0);
  }
  // printf("File %s opened successfully.\n", TEST_FILE);

  pid_t pid = fork();

  if (pid == 0) // child process
  {
    // printf("Child process attempting to read from file.\n");
    bytesRead = read(local_fd, buf, BUF_SIZE - 1);
    if (bytesRead < 0 || bytesRead > (BUF_SIZE - 1)) {
      printf("Failed to read from file\n");
      assert(0);
      return -1;
    }
    // printf("Child process successfully read from file.\n");
    exit(0);
  }
  else if (pid > 0) // parent process
  {
    // printf("Parent process attempting to write to file.\n");
    bytesWritten = write(local_fd, writeData, strlen(writeData));
    if (bytesWritten < 0 || bytesWritten > strlen(writeData)) {
      printf("Failed to write to file\n");
      assert(0);
      return -1;
    }
    // printf("Parent process successfully wrote to file.\n");
  }

  return 0;
}