#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define TEST_FILE "usr/test4.txt"
#define BUF_SIZE  128

//Open and close more local file descriptor and read or write to them
int fdTest() {
  char buf[BUF_SIZE];
  int local_fd;
  ssize_t bytesRead, bytesWritten;

  // Opening usr/test.tx
  local_fd = open(TEST_FILE, O_RDWR);

  if (local_fd < 0) {
    printf("Failed to open file\n");
    assert(0);
    return -1;
  }
  // printf("Local file descriptor: %d\n", local_fd);

  // Opening all files between 'usr/file0.txt' and 'usr/file4.txt
  int fds[5];
  char path[20] = "usr/file0.txt";
  for(int i = 0; i < 5; i++) {
    path[8] = '0' + i;
    fds[i] = open(path, O_RDWR);
    assert(fds[i] >= 0);
    // printf("Opened file: %s --> Local FD: %d\n", path, fds[i]);
  }

  //Writting to usr/test.tx
  char* writeData = ":/";
  bytesWritten = write(local_fd, writeData, strlen(writeData) + 1);
  if (bytesWritten < 0 || bytesWritten > strlen(writeData) + 1) {
    printf("Failed to write to file\n");
    assert(0);
  }

  //Reading to usr/test.tx
  lseek(local_fd, 0, SEEK_SET);
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0 || bytesRead > BUF_SIZE - 1) {
    printf("Failed to read from file\n");
    assert(0);
    return -1;
  }
  buf[bytesRead] = '\0';
  assert(strcmp(buf, ":/") == 0);


  //Closing file
  if (close(local_fd) < 0) {
    printf("Failed to close file\n");
    assert(0);
  }

  //Close the other files
  for(int i = 0; i < 5; i++)
  {
  lseek(fds[i], 0, SEEK_SET);
  bytesRead = read(fds[i], buf, BUF_SIZE - 1);
  if (bytesRead < 0 || bytesRead > BUF_SIZE - 1) {
    printf("Failed to read from file\n");
    assert(0);
    return -1;
  }

    assert(close(fds[i]) == 0);
  }
  
  return 0;
}