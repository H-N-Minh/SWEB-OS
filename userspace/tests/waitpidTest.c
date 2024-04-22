#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>

int main() {
  pid_t pid;
  int status;

  pid = 2; // Create a child process

  printf("%p\n", &status);

  waitpid(pid, &status, 0);

  return 0;
}

