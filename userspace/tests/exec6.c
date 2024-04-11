#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "pthread.h"

int flag1 = 0;
void print_function()
{
  while(1)
  {
    printf(". ");
    flag1 = 1;
  }   
}




//Test: Exec with arguments
int main()
{
    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void*)print_function, NULL);
    assert(rv == 0);

    while(!flag1){}

    const char * path = "usr/exec4_testprogram.sweb";
    char *argv[] = { "Alle meine", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    execv(path, argv);
    assert(0);    //this should never be reached
    return 0;
}