#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define TEST_FILE "usr/test1.txt"
#define BUF_SIZE  128

//Checks if local filedescriptors gets copied and child can read them and if parent can still read/write after child closed descriptor

int forklfds() {
  char buf[BUF_SIZE];
  int local_fd;
  size_t bytesRead, bytesWritten;

  local_fd = open(TEST_FILE, O_RDWR);
 
  if (local_fd < 0) {
    assert(0 && "File descriptor not available");
  }

  //Opening all files between 'usr/file0.txt' and 'usr/file4
  int fds[5];
  char path[20] = "usr/file0.txt";
  for(int i = 0; i < 5; i++) {
    path[8] = '0' + i;
    fds[i] = open(path, O_RDWR);
    assert(fds[i] > 0 && "File descriptor not available");
  }
  
  pid_t pid = fork();

  if (pid == 0) // child process
  {
    if (lseek(local_fd, 0, SEEK_SET) == -1) {
      assert(0 && "lseek failed");
    }

    //Reading to file in child
    bytesRead = read(local_fd, buf, BUF_SIZE);
    if (bytesRead < 0 || bytesRead > BUF_SIZE) {
      assert("Failed to read from file\n" && 0);
    }
    buf[bytesRead] = '\0';

    //Check if the correct data is read
    // printf("Buffer1: %s\n", buf);
    assert(strcmp(buf, ":/ :( :) :( :) Hello World!") == 0);

    //Closing file in child
    if (close(local_fd) < 0) {
      printf("Failed to close file\n");
    }
    exit(0);
  }
  else if (pid > 0) // parent process
  {
    sleep(3); //Wait so that child has time to read and close
  }
  else
  {
    assert(0 && "Fork failed");
  }


  assert(pid != 0 && "Only parent should reach this");
  char* writeData = ":/";
  bytesWritten = write(local_fd, writeData, strlen(writeData)+1);
  if (bytesWritten < 0 || bytesWritten > BUF_SIZE) {
    assert(0 && "Write failed");
  }

  lseek(local_fd, 0, SEEK_SET);
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  if (bytesRead < 0 || bytesRead > BUF_SIZE) {
    assert(0 && "Read failed");
  }

  buf[bytesRead] = '\0';

  //Check if read data that i wrote to the file
  // printf("Buffer: %s\n", buf);
  assert(strcmp(buf, ":/ :( :) :( :) Hello World!:/") == 0);


  if (close(local_fd) < 0) {
    assert(0 && "Closing failed");
  }
  return 0;
}


//Same but with printstatements
int forklfds_debugmode() {
  char buf[BUF_SIZE];
  int local_fd;
  size_t bytesRead, bytesWritten;

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
    assert(fds[i] > 0);
    //printf("Opened file: %s --> Local FD: %d\n", path, fds[i]);
  }


  pid_t pid = fork();

  if (pid == 0) // child process
  {
    printf("Child's local_fd: %d\n", local_fd);

    printf("Reading from file: %s\n", TEST_FILE);
    printf("Before lseek\n");
    if (lseek(local_fd, 0, SEEK_SET) == -1) {
      printf("lseek error\n");
      return -1;
    }
    printf("After lseek, before read\n");
    bytesRead = read(local_fd, buf, BUF_SIZE - 1);
    printf("After read\n");
    if (bytesRead < 0 || bytesRead > BUF_SIZE) {
      printf("Failed to read from file\n");
      return -1;
    }
    buf[BUF_SIZE] = '\0';
    printf("Read from file: %s\n", buf);

    printf("Closing file: %s\n", TEST_FILE);
    if (close(local_fd) < 0) {
      printf("Failed to close file\n");
      return -1;
    }
    printf("File closed successfully\n");
    exit(0);
  }
  else if (pid > 0) // parent process
  {
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
  bytesWritten = write(local_fd, writeData, strlen(writeData)+1);
  printf("bytes written %ld\n\n", bytesWritten);
  printf("After write operation in %s process, returned: %zd\n", (pid == 0 ? "Child":"parent"), bytesWritten);
  if (bytesWritten < 0 || bytesWritten > BUF_SIZE) {
    printf("Failed to write to file\n");
    return -1;
  }

  printf("Reading from file: %s\n", TEST_FILE);
  lseek(local_fd, 0, SEEK_SET);
  bytesRead = read(local_fd, buf, BUF_SIZE - 1);
  printf("bytes read %ld\n\n", bytesRead);
  if (bytesRead < 0 || bytesRead > BUF_SIZE) {
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

