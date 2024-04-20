#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"

#define TEST_FILE "usr/test.txt"
#define BUF_SIZE  128

int main() {
  char buf[BUF_SIZE];
  memset(buf, 0, BUF_SIZE);

  printf("Opening file: %s\n", TEST_FILE);

  int local_fd = open(TEST_FILE, O_RDWR);
  if (local_fd < 0) {
    printf("Failed to open file");
    return local_fd;
  }

  printf("Local file descriptor: %d\n", local_fd);

  int fds[5];
  char path[20] = "usr/file0.txt";
  for(int i = 0; i < 5; i++) {
    path[8] = '0' + i;
    fds[i] = open(path, O_RDWR);
    printf("Opened file: %s --> Local FD: %d\n", path, fds[i]);
  }




  char* writeData = ":/";
  ssize_t bytesWritten = write(local_fd, writeData, strlen(writeData));
  if (bytesWritten < 0) {
    printf("Failed to write to file");
    return -1;
  }

  lseek(local_fd, 0, SEEK_SET);
  ssize_t bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0) {
    printf("Failed to read from file");
    return -1;
  }
  buf[bytesRead] = '\0';
  printf("Read from file: %s\n", buf);

  if (close(local_fd) < 0) {
    printf("Failed to close file");
    return -1;
  }
  return 0;
}