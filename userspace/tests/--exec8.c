/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


#include "assert.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"


int exec8_1()
{
    //more than one page of arguments

    const char * path = "usr/exec_testprogram.sweb";
    char* long_word = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj";
    
    assert(strlen(long_word) == 1000);
    char *argv[] = {"8", long_word, long_word, long_word, long_word, long_word, long_word, (char *)0 };
    execv(path, argv);
    assert(0);

    return 0;
}

int main()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    return 0;
  } 
  else //parent
  {
    exec8_1();
    assert(0);
    return 0;
  }
}