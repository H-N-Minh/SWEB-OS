#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "pthread.h"

#define PRINT_DESCRIPTION 1

extern int pc1();
extern int pc2();
extern int pc3();
extern int pc4();
extern int pc5();
extern int pc6();
extern int pc7();
extern int pc8();
extern int pc9();
extern int pc10();
extern int pc11();
extern int pc12();
extern int pc13();


void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("PthreadChreate:Test %d successful!\n", testnumber);
            printf("Description: %s\n", description);
            printf("_____________________________________________________________\n\n");
        }
        (*successful_tests)++;
    }
    else
    {
        
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("PthreadChreate:Test %d fail!\n", testnumber);
            printf("Description: %s\n", description);
            printf("_____________________________________________________________\n\n");
            exit(-3);
        }
    }
}

int childMain()
{
    //If maxkernel heap is reached when testing all together comment out some tests
    int successful_tests = 0;
    int number_of_tests = 0;
    int rv;
    

    // rv = pc1();  //sanity checks: wrong para
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "sanity checks");

    // rv = pc2();  //simple pthread_create and check if thread id gets set
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "simple pthread_create and check if thread id gets set");

    // rv = pc3();  //starting 250 threads
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "starting 250 threads");

    // rv = pc4();  //check if two running threads have different id
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "check if two running threads have different id");

    // rv = pc5();  //pthread create with argument
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "pthread create with argument");

    // rv = pc6();  //running 250 simultaniously
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "running 250 simultaniously");

    // rv = pc7();  //calling pthread_create inside pthread_create inside pthread_create
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "calling pthread_create inside pthread_create inside pthread_create");

    // rv = pc8();  //invalid userspace addresss as thread_id
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "invalid userspace addresss as thread_id");

    // rv = pc9();  // 100 threads calling 100 pcreate at the same time, and the parameters and return value should be unique
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, 
    //         "100 threads calling 100 pcreate at the same time, Also test the parameters and return value of all threads shouldnt get mixed up");

    // // rv = pc10();  // each thread has its own stack but they can still access each other's stack
    // // number_of_tests++;
    // // check_return_value(number_of_tests, rv, &successful_tests, "each thread has its own stack but they can still access each other's stack");

    // rv = pc11();  //testing pthread_create attributes (detach):
    // //pthread create with wrong attributes
    // //using pthread attributes twice for different threads
    // //destroying twice
    // //using a destroyed attribute
    // //destroy attribute with address not in userspace
    // //init attribute with address not in userspace
    // //detachstate after init should be joinable
    // //setting pthread_attribute to detach
    // //join and cancel if state is detached
    // //set and get if attributes is not initalized  
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "testing pthread_create attributes (detach):");

    // rv = pc12();  //test if thread pthread_self()
    // number_of_tests++;
    // check_return_value(number_of_tests, rv, &successful_tests, "test if pthread_self() works");
    printf("hey\n");
    rv = pc13();  //test thousands of threads
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "create and destroy 6000 threads");

    if(successful_tests == number_of_tests)
    {
        printf("\n\n===  All pthreadCreate() testcases successful  ===\n");
    }
    else
    {
        printf("\n\n===  pthreadCreate() testcases fail  ===\n");
        exit(-3);
    }

    exit(0);
    return 0;
}

int main()
{
    // pid_t pid = fork();
    // if (pid == 0) {
        int child_exit_code = childMain();
    //     exit(child_exit_code);
    // } 
    // else
    // {
    //     int status;
    //     waitpid(pid, &status, 0);
    //     if (status == -3)
    //     {
    //         printf("Some test(s) failed\n");
    //     }
    //     else if (status != 0 && status != -1)
    //     {
    //         printf("process crashed with error code %d\n", status);
    //     }
        
    //     for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
    //     {
    //         /* code */
    //     }

    //     int num = get_thread_count();
    //     if (num == 7 || num == 6)
    //     {
    //         printf("===  All threads are destroyed correctly  ===\n");
    //         return 0;
    //     }
    //     else
    //     {
    //         printf("===  %d threads are still alive===  \n", num);
    //         return -1;
    //     }
        
    // }
    return 0;
}
                    