#include <stdio.h>
#include <unistd.h>

int main()
{
  pid_t pid;

  pid = fork();

  if (pid < 0) {
    return 1;
  }
  else if (pid == 0)
  {
    printf("Child process is executing n");
  }
  else
  {
    printf("Parent process is executing \n");
    printf("Child process PID: %ld\n", pid);
  }
  return 0;
}
