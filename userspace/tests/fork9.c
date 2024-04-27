#include <stdio.h>
#include <unistd.h>

int global_var = 10;
int main() {
  int stack_var = 20;
  pid_t pid = fork();

  if (pid == -1) {
    printf("Fork failed");
    return 1;
  } else if (pid == 0) {
    printf("Child process - Before modification: global_var = %d, stack_var = %d\n", global_var, stack_var);

    global_var = 50;
    stack_var = 60;

    printf("Child process - After modification: global_var = %d, stack_var = %d\n", global_var, stack_var);
  } else {
    printf("Parent process - Before modification: global_var = %d, stack_var = %d\n", global_var, stack_var);

    global_var = 30;
    stack_var = 40;

    printf("Parent process - After modification: global_var = %d, stack_var = %d\n", global_var, stack_var);
  }

  return 0;
}
