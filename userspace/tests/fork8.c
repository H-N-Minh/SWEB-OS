/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 69]
disabled: false
*/
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int value = 0;

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
    value += 1;
    printf("Child 1 value: %d\n", value);
    assert(value == 1);
    child1 = fork();
    if (child1 == 0)
    {
      printf("Grandchild 1 process\n");
      value += 2;
      printf("Grandchild 1 value: %d\n", value);
      assert(value == 3);
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
      //is executed by the second child process
      printf("Child 2 process\n");
      value += 3; // Increment value by 3
      printf("Child 2 value: %d\n", value);
      assert(value == 3); // Check if value is 3
      child2 = fork();
      if (child2 == 0)
      {
        printf("Grandchild 2 process\n");
        value += 4;
        printf("Grandchild 2 value: %d\n", value);
        assert(value == 7); // Check if value is 7
      }
    }
    else
    {
      printf("Parent process\n");
      value += 5;
      printf("Parent value: %d\n", value);
      assert(value == 5);
    }
  }
  return 0;
}
