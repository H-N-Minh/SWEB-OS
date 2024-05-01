#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include <wait.h>

#define FILENAME "usr/forklfds4.txt"
#define BUFFER_SIZE 256

int main() {
  pid_t pid;
  int status;
  int file_descriptor;
  char buffer[BUFFER_SIZE];

  pid = fork();

  if (pid < 0)
  {
    printf("Fork failed\n");
    return 1;
  }
  else if (pid == 0)
  {
    printf("Child process is writing to file...\n");

    file_descriptor = open(FILENAME, O_RDWR | O_CREAT);
    if (file_descriptor < 0)
    {
      printf("Error opening file for writing\n");
      return 1;
    }

    char* random = "Hello from the child process!\n";
    write(file_descriptor, random, strlen(random));
    close(file_descriptor);

    printf("Child process has written to file\n");
  }
  else
  {
    waitpid(pid, &status, 0);
    printf("Parent process is reading from file...\n");

    file_descriptor = open(FILENAME, O_RDONLY);
    if (file_descriptor < 0)
    {
      printf("Error opening file for reading\n");
      return 1;
    }

    read(file_descriptor, buffer, BUFFER_SIZE);
    close(file_descriptor);

    printf("Data read from file by parent process: %s", buffer);
  }

  return 0;
}