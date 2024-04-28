#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int sleep1();
extern int sleep2();
extern int clock1();
extern int clock2();


void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("SleepClock:Test %d successful!\n", testnumber);
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
            printf("SleepClock:Test %d fail!\n", testnumber);
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

    rv = sleep1();          
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Simple sleep");

    rv = sleep2();          
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Sleep in new thread");

    rv = clock1();          
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Clock with and without sleep");

    rv = clock2();          
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Clock with two processes");


    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll SleepClock testcases successful\n");
    }
    else
    {
        printf("\n\nSleepClock testcases fail\n");
        assert(0);
    }

}                          