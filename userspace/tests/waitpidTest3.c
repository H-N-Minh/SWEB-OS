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
    printf("First child process running...\n");
    pid_t pid2 = fork();

    if (pid2 < 0)
    {
      printf("Second Fork failed\n");
      return 1;
    }
    else if (pid2 == 0)
    {
      printf("Child of child process running...\n");
    }
    else
    {
      waitpid(pid2, &status, 0);
      printf("First child process waiting for its child to terminate...\n");
    }
  }
  else
  {
    waitpid(pid, &status, 0);
    printf("Parent process waiting for first child to terminate...\n");
  }

  return 0;
}
