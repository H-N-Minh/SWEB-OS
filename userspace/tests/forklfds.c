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

  printf("Opening file: %s\n", TEST_FILE);
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    return -1;
  }
  printf("Local file descriptor: %d\n", local_fd);

  printf("Opening all files between 'usr/file0.txt' and 'usr/file4.txt'\n");
  int fds[5];
  char path[20] = "usr/file0.txt";
  for(int i = 0; i < 5; i++) {
    path[8] = '0' + i;
    fds[i] = open(path, O_RDWR);
    printf("Opened file: %s --> Local FD: %d\n", path, fds[i]);
  }


  pid_t pid = fork();

  if (pid == 0) // child process
  {
    printf("Inside child process\n");
    printf("Child's local_fd: %d\n", local_fd);

    printf("Reading from file: %s\n", TEST_FILE);
    printf("Before lseek\n");
    if (lseek(local_fd, 0, SEEK_SET) == -1) {
      printf("lseek error");
      return -1;
    }
    printf("After lseek, before read\n");
    bytesRead = read(local_fd, buf, BUF_SIZE - 1);
    printf("After read\n");
    if (bytesRead < 0) {
      printf("Failed to read from file\n");
      return -1;
    }
    buf[bytesRead] = '\0';
    printf("Read from file: %s\n", buf);

    printf("Closing file: %s\n", TEST_FILE);
    if (close(local_fd) < 0) {
      printf("Failed to close file\n");
      return -1;
    }
    printf("File closed successfully\n");
  }
  else if (pid > 0) // parent process
  {
    printf("Inside parent process\n");
    printf("Parent's local_fd: %d\n", local_fd);
    sleep(1);
  }
  else
  {
    printf("Fork failed\n");
    return -1;
  }

  printf("Writing to file: %s\n", TEST_FILE);
  char* writeData = ":/";
  printf("Before write operation in %s process, FD: %d\n", (pid == 0 ? "Child":"parent"), local_fd);
  bytesWritten = write(local_fd, writeData, strlen(writeData));
  printf("After write operation in %s process, returned: %zd\n", (pid == 0 ? "Child":"parent"), bytesWritten);
  if (bytesWritten < 0) {
    printf("Failed to write to file\n");
    return -1;
  }

  printf("Reading from file: %s\n", TEST_FILE);
  lseek(local_fd, 0, SEEK_SET);
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0) {
    printf("Failed to read from file\n");
    return -1;
  }
  buf[bytesRead] = '\0';
  printf("Read from file: %s\n", buf);

  printf("Closing file: %s\n", TEST_FILE);
  if (close(local_fd) < 0) {
    printf("Failed to close file\n");
    return -1;
  }
  printf("File closed successfully\n");

  return 0;
}