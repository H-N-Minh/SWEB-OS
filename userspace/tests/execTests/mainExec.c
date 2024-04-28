#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int exec1();
extern int exec2();
extern int exec3();
extern int exec4();
extern int exec5();
extern int exec6();
extern int exec7();
extern int exec8();
extern int exec9();
extern int exec10();
extern int exec11();

void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("Exec:Test %d successful!\n", testnumber);
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
            printf("Exec:Test %d fail!\n", testnumber);
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
    

    rv = exec1();  //Exec without NULL as argument
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec without NULL as argument");

    rv = exec2();  //exec with wrong path   
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "exec with wrong path");

    rv = exec3();  //Exec with many arguments
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec with many arguments");

    rv = exec4();  //Exec with arguments
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec with arguments");

    rv = exec5();  //Exec without arguments
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec without arguments");

    rv = exec6(); //Exec where another function is alive when exec
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec where another function is alive when exec");

    rv = exec7(); //Exec in pthread create
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec in pthread create");

    rv = exec8();  //Exec with two pages of arguments
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec with two pages of arguments");

    rv = exec9(); //Exec in multiple pthread create
    number_of_tests++;
    sleep(3);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec in multiple pthread create");

    rv = exec10(); //Exec with growing stack
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec with growing stack");

    rv = exec11(); //Exec in multiple forks
    number_of_tests++;
    sleep(1);
    check_return_value(number_of_tests, rv, &successful_tests, "Exec in multiple forks");

    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll exec testcases successful\n");
    }
    else
    {
        printf("\n\nexec testcases fail\n");
        assert(0);
    }




}

                            

                      