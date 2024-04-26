#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>

int main()
{
  pid_t pid;
  int status;

  // Create a child process
  pid = fork();

  if (pid < 0)
  {
    printf("Fork faild\n");
  }
  else if (pid == 0)
  {
    sleep(1);
    printf("Child process running...\n");
  }
  else
  {
    waitpid(pid, &status, 0);
    printf("Parent process waiting for child to terminate...\n");
  }

  return 0;
}

