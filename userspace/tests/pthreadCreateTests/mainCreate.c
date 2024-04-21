#include "stdio.h"
#include "assert.h"

extern int pc1();
extern int pc2();
extern int pc3();
extern int pc4();
extern int pc5();
extern int pc6();

void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        printf("-------------------------------------------------------------\n");
        printf("PthreadChreate:Test %d successful!\n", testnumber);
        printf("Description: %s\n", description);
        printf("_____________________________________________________________\n\n");
        (*successful_tests)++;
    }
    else
    {
        printf("-------------------------------------------------------------\n");
        printf("PthreadChreate:Test %d successful!\n", testnumber);
        printf("Description: %s\n", description);
        printf("_____________________________________________________________\n\n");
    }
}

int main()
{
    int successful_tests = 0;
    int number_of_tests = 0;
    int rv;
    

    rv = pc1();  //sanity checks
    number_of_tests++;
    check_return_value(1, rv, &successful_tests, "sanity checks");

    rv = pc2();  //simple pthread_create and check if thread id gets set
    number_of_tests++;
    check_return_value(2, rv, &successful_tests, "simple pthread_create and check if thread id gets set");

    rv = pc3();  //starting 250 threads
    number_of_tests++;
    check_return_value(3, rv, &successful_tests, "starting 250 threads");

    rv = pc4();  //check if to running threads have different id
    number_of_tests++;
    check_return_value(4, rv, &successful_tests, "check if to running threads have different id");

    rv = pc5();  //pthread create with simple argument
    number_of_tests++;
    check_return_value(5, rv, &successful_tests, "pthread create with simple argument");

    rv = pc6();  //running 250 simultaniously
    number_of_tests++;
    check_return_value(6, rv, &successful_tests, "running 250 simultaniously");

    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll pthreadCreate() testcases successful\n");
    }
    else
    {
        printf("\n\npthreadCreate() testcases fail\n");
        assert(0);
    }

}
                    