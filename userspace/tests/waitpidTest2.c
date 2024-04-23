#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main()
{
  pid_t pid;
  int status;

  pid = fork();

  if (pid < 0)
  {
    printf("Fork failed\n");
    return 1;
  }
  else if (pid == 0)
  {
    printf("Child process 1 running...\n");
  }
  else
  {
    pid_t pid2 = fork();

    if (pid2 < 0)
    {
      printf("Fork failed\n");
      return 1;
    }
    else if (pid2 == 0)
    {
      printf("Child process 2 running...\n");
    }
    else
    {
      waitpid(pid2, &status, 0);
      printf("Parent process waiting for child 2 to terminate...\n");

      waitpid(pid, &status, 0);
      printf("Parent process waiting for child 1 to terminate...\n");
    }
  }

  return 0;
}