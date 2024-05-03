#include "unistd.h"
#include <stdio.h>
#include <pthread.h>
#include "wait.h"

#define MSG_SIZE 100

void write_to_pipe(int arg)
{
  int write_fd = arg;
  char msg[MSG_SIZE] = "Testing pipe communication!";
  write(write_fd, msg, sizeof(msg));
}

void read_from_pipe(int arg)
{
  int read_fd = arg;
  char buf[MSG_SIZE] = {0};
  read(read_fd, buf, sizeof(buf));
  printf("Read from pipe: %s\n", buf);
}

int main(){
  int file_descriptor_array[2];
  pipe(file_descriptor_array);
  int read_fd = file_descriptor_array[0];
  int write_fd = file_descriptor_array[1];

  int pid = fork();
  int status;

  if (pid < 0)
  {
  }
  else if (pid == 0)
  {
    write_to_pipe(write_fd);
  }
  else //Parent
  {
    waitpid(pid, &status, 0);
    read_from_pipe(read_fd);
  }

  return 0;
}
