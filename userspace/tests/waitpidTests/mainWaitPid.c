#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int waitpid1();
extern int waitpid2();
extern int waitpid3();
extern int waitpid4();


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

    rv = waitpid2();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "");

    rv = waitpid3();  
    if(rv == 5)
    {
        exit(0);
    }
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "");

    rv = waitpid4();  
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "");



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
                    