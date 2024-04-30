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

  printf("Writing to file: %s\n", TEST_FILE);
  char* writeData = ":/";
  bytesWritten = write(local_fd, writeData, strlen(writeData));
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