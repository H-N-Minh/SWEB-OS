#include "stdio.h"
#include "pthread.h"
int main(int argc, char *argv[]) {
  printf("Threadcount: %d", get_thread_count());
  return 0;
}





//#include <unistd.h>
//#include <stdio.h>
//#include <string.h>

//test für pipe()
//void testPipe() {
//  int fd[2];
//  pid_t childpid;
//
//  char string[] = "Hallo, das ist ein Test.";
//  char readbuffer[80];
//
//  pipe(fd);
//
//  if((childpid = fork()) == -1) {
//    perror("fork");
//    exit(1);
//  }
//
//  if(childpid == 0) {
//    close(fd[0]);
//    write(fd[1], string, strlen(string)+1);
//    exit(0);
//  } else {
//
//    close(fd[1]);
//    read(fd[0], readbuffer, sizeof(readbuffer));
//    printf("Empfangene String: %s\n", readbuffer);
//  }
//}
//
//int main() {
//  testPipe();
//  return 0;
//}