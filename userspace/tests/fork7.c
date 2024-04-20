/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 69]
disabled: false
*/
//This test should be call fork after fork

#include <stdio.h>
#include <unistd.h>

int main() {
  pid_t child1, child2;

  child1 = fork();

  if (child1 < 0)
  {
    printf("First fork failed\n");
    return 1;
  }
  else if (child1 == 0)
  {
    printf("Child 1 process\n");
    // Second fork in the first child process
    child1 = fork();
    if (child1 < 0)
    {
      printf("Second fork in Child 1 failed\n");
      return 1;
    }
    else if (child1 == 0)
    {
      printf("Grandchild 1 process\n");
    }
    else
    {
      printf("Child 1 process\n");
    }
  }
  else
  {
    child2 = fork();

    if (child2 < 0)
    {
      printf("Second fork failed\n");
      return 1;
    }
    else if (child2 == 0)
    {
      printf("Child 2 process\n");
      // Second fork in the second child process
      child2 = fork();
      if (child2 < 0)
      {
        printf("Second fork in Child 2 failed\n");
        return 1;
      }
      else if (child2 == 0)
      {
        printf("Grandchild 2 process\n");
      }
      else
      {
        printf("Child 2 process\n");
      }
    }
    else
    {
      printf("Parent process\n");
    }
  }

  return 0;
}
