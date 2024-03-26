#include "fcntl.h"
#include "string.h"
#include "stdio.h"

#define TEST_FILE "/home/fabian/Desktop/bss24c1/userspace/tests/binary.bin"
#define BUF_SIZE  128

/**
 * @brief Main function for opening a file
 *
 * This function opens a file and prints a message indicating whether the
 * file was successfully opened or not.
 *
 * @return 0 on success, negative value on failure
 *
 * @remarks This function uses the following external functions:
 * - memset: to initialize the buffer with zeros
 * - printf: to print the message
 * - open: to open the file
 *
 */
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