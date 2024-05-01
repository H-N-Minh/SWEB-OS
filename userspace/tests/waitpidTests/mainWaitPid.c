#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int waitpid1();
extern int waitpid2();
extern int waitpid3();
extern int waitpid4();
extern int waitpid5();
extern int waitpid6();
extern int waitpid7();


void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("WaitPid:Test %d successful!\n", testnumber);
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
            printf("WaitPid:Test %d fail!\n", testnumber);
            printf("Description: %s\n", description);
            printf("_____________________________________________________________\n\n");
        }
    }
}

int main()
{
    int successful_tests = 0;
    int number_of_tests = 0;
    int rv;
    
    //simple waitpid where parent waits on running child
    rv = waitpid1();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "simple waitpid where parent waits on running child");

    //Create two childs in the parents and waitpid for both
    rv = waitpid2();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Create two childs in the parents and waitpid for both");


    //Fork and waitpid and in the child again waitpid
    rv = waitpid3();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Fork and waitpid and in the child again waitpid");

    //Sanity check:
    //Waiting for nonexisting process
    //Waiting for pid smaller equal 0 (not supported by our implementation)
    //Waitpid with no status provided
    //Waitpid with not userspace address as statusptr
    //Waitpid with nonexisting option (options also not supported)
    rv = waitpid4();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Sanity checks");

    //Waitpid with exec
    rv = waitpid5();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Waitpid with exec");

    //Waitpid with multiple threads
    rv = waitpid6();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Waitpid with multiple threads");


    //Waitpid with multiple threads and exec
    rv = waitpid7();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Waitpid with multiple threads and exec");


    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll waitpid() testcases successful\n");
    }
    else
    {
        printf("\n\nwaitpid() testcases fail\n");
        assert(0);
    }
}
                    