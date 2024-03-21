#include "fcntl.h"
#include "unistd.h"
#include "string.h"
#include "stdio.h"

#define TEST_FILE "/home/fabian/Desktop/bss24c1/userspace/tests/test.txt"
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


  return 0;
}