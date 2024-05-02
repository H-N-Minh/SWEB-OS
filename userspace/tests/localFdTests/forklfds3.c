#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "wait.h"
#include "assert.h"

#define FILENAME "usr/forklfds4.txt"
#define BUFFER_SIZE 256

//Checks if parent can read the data that child wrote to file before

int forklfds3() {
  pid_t pid;
  int status;
  int file_descriptor;
  char buffer[BUFFER_SIZE];

  pid = fork();

  if (pid < 0)
  {
    printf("Fork failed\n");
    assert(0);
    return 1;
  }
  else if (pid == 0)
  {
    //Child writting to file
    // printf("Child process is writing to file...\n");

    file_descriptor = open(FILENAME, O_RDWR | O_CREAT);
    if (file_descriptor < 0)
    {
      printf("Error opening file for writing\n");
      assert(0);
      return 1;
    }

    char* random = "Hello from the child process!";
    ssize_t rv = write(file_descriptor, random, strlen(random) + 1);
    if(rv < 0 || rv >  strlen(random) + 1)
    {
      assert(0);
    }
    assert(close(file_descriptor) == 0);

    exit(0);
    // printf("Child process has written to file\n");
  }
  else
  {
    waitpid(pid, &status, 0);
    // printf("Parent process is reading from file...\n");

    file_descriptor = open(FILENAME, O_RDONLY);
    if (file_descriptor < 0)
    {
      printf("Error opening file for reading\n");
      assert(0);
      return 1;
    }

    ssize_t rv = read(file_descriptor, buffer, BUFFER_SIZE);
    if(rv < 0 || rv > BUFFER_SIZE)
    {
      assert(0);
    }
    buffer[rv] = NULL;
    assert(close(file_descriptor) == 0);
    assert(strcmp(buffer, "Hello from the child process!") == 0);
    // printf("Data read from file by parent process: %s", buffer);
  }

  return 0;
}