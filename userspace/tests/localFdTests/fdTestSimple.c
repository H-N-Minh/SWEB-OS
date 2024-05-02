#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define TEST_FILE "usr/test2.txt"
#define BUF_SIZE  128

//Checks if simple read and write works with local fd
int fdTestSimple() {
  char buf[BUF_SIZE];
  int local_fd;
  ssize_t bytesRead, bytesWritten;

  //Opening file usr/test2.txt
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    assert(0);
  }
  // printf("Local file descriptor: %d\n", local_fd);

  //"Writing to file user/test2.txt
  char* writeData = "Hello";
  bytesWritten = write(local_fd, writeData, strlen(writeData));
     
  if (bytesWritten < 0 || bytesWritten > strlen(writeData)) {
    printf("Failed to write to file\n");
    assert(0);
  }

  // printf("Reading from file: %s\n", TEST_FILE);
  
  lseek(local_fd, 0, SEEK_SET);
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0 || bytesRead > BUF_SIZE - 1) {
    printf("Failed to read from file\n");
    assert(0);
  }
  buf[21] = '\0';
  assert(strcmp(buf, "Hello wie geht es dir") == 0);

  //Close file
  if (close(local_fd) < 0) {
    printf("Failed to close file\n");
    assert(0);
  }
  return 0;
}