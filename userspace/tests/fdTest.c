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

  int fd = open(TEST_FILE, O_RDWR);
  if (fd < 0) {
    printf("Failed to open file");
    return fd;
  }

  char* writeData = ":/";
  ssize_t bytesWritten = write(fd, writeData, strlen(writeData));
  if (bytesWritten < 0) {
    printf("Failed to write to file");
    return -1;
  }

  lseek(fd, 0, SEEK_SET);
  ssize_t bytesRead = read(fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0) {
    printf("Failed to read from file");
    return -1;
  }
  buf[bytesRead] = '\0';
  printf("Read from file: %s\n", buf);

  if (close(fd) < 0) {
    printf("Failed to close file");
    return -1;
  }
  return 0;
}