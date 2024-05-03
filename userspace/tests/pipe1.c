#include "unistd.h"
#include <stdio.h>
#include <pthread.h>
#include "wait.h"
#include "assert.h"


#define MSG_SIZE 100

void write_to_pipe(int arg)
{
  int write_fd = arg;
  char msg[MSG_SIZE] = "Testing pipe communication!";
  int size = sizeof(msg);
  int rv = write(write_fd, msg, sizeof(msg));
  assert(rv == size);
}

void read_from_pipe(int arg)
{
  int read_fd = arg;
  char buf[MSG_SIZE] = {0};
  int size = sizeof(buf);
  int rv = read(read_fd, buf, sizeof(buf));
  assert(rv == size);
  printf("Read from pipe: %s\n", buf);
}

int main(){
  int file_descriptor_array[2];
  int rv = pipe(file_descriptor_array);
  assert(rv == 0);

  int pid = fork();
  int status;

  if (pid < 0)
    return 1;
  else if (pid == 0)
    write_to_pipe(file_descriptor_array[1]);
  else
  {
    waitpid(pid, &status, 0);
    read_from_pipe(file_descriptor_array[0]);
  }
  return 0;
}
