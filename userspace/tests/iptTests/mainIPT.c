#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

extern int ipt1();
extern int ipt2();
extern int ipt3();
extern int ipt4();

// void printDebugInfo()
// {
//   while(1)
//   {
//     getIPTInfos();
//     sleep(1); 
//   }
// }

int main()
{
    // pthread_t thread_id;
    // pthread_create(&thread_id, NULL, (void*)printDebugInfo, NULL);

    // ipt1();
    // printf("first done\n");
    // ipt2();
    // printf("second done\n");
    // ipt3();
    // printf("third done\n");
   ipt4(); //ipt4 is too big i think
   printf("fourth done\n");


    // printf("\n\nipt testcases successful\n");
}

