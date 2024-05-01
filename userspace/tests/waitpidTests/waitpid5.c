#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"

//Waitpid with exec
int waitpid5()
{
  pid_t pid;
  int status;

  pid = fork();

  assert(pid >= 0);

  if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "4", "Alle meine Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    execv(path, argv);
    assert(0);    //this should never be reached

  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 0);

  }

  pid = fork();

  assert(pid >= 0);

  if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char* long_word = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj";
    
    char *argv[] = {"8", long_word, long_word, long_word, long_word, long_word, long_word, (char *)0 };
    execv(path, argv);
    assert(0);

  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 0);

  }

  return 0;
}

