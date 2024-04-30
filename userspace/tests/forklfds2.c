#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"

#define TEST_FILE "usr/test.txt"
#define BUF_SIZE  128

int main() {
  char buf[BUF_SIZE];
  int local_fd;
  ssize_t bytesRead, bytesWritten;
  char* writeData = ":/";

  printf("Opening file: %s\n", TEST_FILE);
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    return -1;
  }
  printf("File %s opened successfully.\n", TEST_FILE);

  pid_t pid = fork();

  if (pid == 0) // child process
  {
    printf("Child process attempting to read from file.\n");
    bytesRead = read(local_fd, buf, BUF_SIZE - 1);
    if (bytesRead < 0) {
      printf("Failed to read from file\n");
      return -1;
    }
    printf("Child process successfully read from file.\n");
  }
  else if (pid > 0) // parent process
  {
    printf("Parent process attempting to write to file.\n");
    bytesWritten = write(local_fd, writeData, strlen(writeData));
    if (bytesWritten < 0) {
      printf("Failed to write to file\n");
      return -1;
    }
    printf("Parent process successfully wrote to file.\n");
  }

  return 0;
}