#include "unistd.h"
#include <stdio.h>

int main(void) {
  int file_descriptor_array[2];
  pipe(file_descriptor_array);
  int read_fd = file_descriptor_array[0];
  int write_fd = file_descriptor_array[1];

  char msg[] = "Testing pipe IPC";
  char buf[100] = {0};

  write(write_fd, msg, sizeof(msg));
  read(read_fd, buf, sizeof(buf));

  printf("Read from pipe: %s\n", buf);

  return 0;
}