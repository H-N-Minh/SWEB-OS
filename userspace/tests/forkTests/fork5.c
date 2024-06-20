#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

#define MAX_FORK 200     // without swaping, can fork around 150 times max

// this tests multiple nested forks. each of them should has their own version of the variable x
// Exactly same as fork4, except: the child after fork dies immediately, while the same parent will 
// be used to fork again
int fork5() {  
    size_t x = 0;
    // create 100 child processes
    for (int i = 0; i < MAX_FORK; i++) {
        assert(x == i  && "each process should have its own unique value of x");
        pid_t pid = fork();
        if (pid > 0) {
            x += 1;      
            continue;    // parent continues to fork
        } 
        else if (pid == 0) {   // child dies
            assert(x == i  && "value x of the child should not be increased after fork");
            return CHILD_SUCCESS; 
        } 
        else {
            return -1;
        }
    }

    // only 1 last process would reach here
    assert(x == MAX_FORK  && "parent process (the 100th process) should now have x = 100");
    return PARENT_SUCCESS;
}


// #include "stdio.h"
// #include "types.h"
// #include "unistd.h"
// #include "pthread.h"
// #include "assert.h"
// #include "sched.h"

// #define PARENT_SUCCESS 0    // parent process returns 0 on success
// #define CHILD_SUCCESS 69    // child process returns 69 on success

// #define MAX_FORK 10     // without swaping, can fork around 150 times max


// #define MEGABYTE 1048576
// #define PAGESIZE 4096

// #define ELEMENTS_IN_ARRAY MEGABYTE / 8
// #define PAGES_IN_ARRAY MEGABYTE/PAGESIZE


// size_t big_array[MEGABYTE/8];  //5 Megabyes

// // this tests multiple nested forks. each of them should has their own version of the variable x
// // Exactly same as fork4, except: the child after fork dies immediately, while the same parent will 
// // be used to fork again
// int fork5() 
// {  
//     size_t x = 0;
//     for (int i = 0; i < MAX_FORK; i++) {
//         assert(x == i  && "each process should have its own unique value of x");
//         pid_t pid = fork();
//         if (pid > 0) 
//         {
//             x += 1;      
//             continue;    // parent continues to fork
//         } 
//         else if (pid == 0) 
//         {   // child dies
//             for(int i = 0; i < PAGES_IN_ARRAY; i++)
//             {
//                 printf("%d\n", i);
//                 big_array[i * (PAGESIZE / 8)] = (size_t)i;
//             }
//             for(int i = 0; i < PAGES_IN_ARRAY; i++)
//             {
//                 printf("_%d\n", i);
//                 assert(big_array[i * (PAGESIZE / 8)] == i);
//             }
//             return CHILD_SUCCESS; 
//         } 
//         else 
//         {
//             return -1;
//         }
//     }

//     // only 1 last process would reach here
//     assert(x == MAX_FORK  && "parent process (the 100th process) should now have x = 100");
//     return PARENT_SUCCESS;
// }