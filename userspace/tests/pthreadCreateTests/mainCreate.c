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
        }
    }
}

int childMain()
{
    int successful_tests = 0;
    int number_of_tests = 0;
    int rv;
    

    rv = pc1();  //sanity checks: wrong para
    number_of_tests++;
    check_return_value(1, rv, &successful_tests, "sanity checks");

    rv = pc2();  //simple pthread_create and check if thread id gets set
    number_of_tests++;
    check_return_value(2, rv, &successful_tests, "simple pthread_create and check if thread id gets set");

    rv = pc3();  //starting 250 threads
    number_of_tests++;
    check_return_value(3, rv, &successful_tests, "starting 250 threads");

    rv = pc4();  //check if two running threads have different id
    number_of_tests++;
    check_return_value(4, rv, &successful_tests, "check if two running threads have different id");

    rv = pc5();  //pthread create with argument
    number_of_tests++;
    check_return_value(5, rv, &successful_tests, "pthread create with argument");

    rv = pc6();  //running 250 simultaniously
    number_of_tests++;
    check_return_value(6, rv, &successful_tests, "running 250 simultaniously");

    rv = pc7();  //calling pthread_create inside pthread_create inside pthread_create
    number_of_tests++;
    check_return_value(7, rv, &successful_tests, "calling pthread_create inside pthread_create inside pthread_create");

    rv = pc8();  //calling pthread_create inside pthread_create inside pthread_create
    number_of_tests++;
    check_return_value(8, rv, &successful_tests, "invalid userspace addresss as thread_id");

    rv = pc9();  // 100 threads calling 100 pcreate at the same time, and the parameters and return value should be unique
    number_of_tests++;
    check_return_value(9, rv, &successful_tests, 
            "100 threads calling 100 pcreate at the same time, Also test the parameters and return value of all threads shouldnt get mixed up");


    if(successful_tests == number_of_tests)
    {
        printf("\n\n===  All pthreadCreate() testcases successful  ===\n");
    }
    else
    {
        printf("\n\n===  pthreadCreate() testcases fail  ===\n");
        assert(0);
    }

    return 0;
}

int main()
{
    pid_t pid = fork();
    if (pid == 0) {
        int child_exit_code = childMain();
        exit(child_exit_code);
    } 
    {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0)
        {
            printf("Testing crashed with exit code %d\n", status);
        }
        
        for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
        {
            /* code */
        }

        int num = get_thread_count();
        if (num == 4 || num == 6)
        {
            printf("===  All threads are destroyed correctly  ===\n");
            return 0;
        }
        else
        {
            printf("===  %d threads are still alive===  \n", num);
            return -1;
        }
        
    }
    return 0;
}
                    