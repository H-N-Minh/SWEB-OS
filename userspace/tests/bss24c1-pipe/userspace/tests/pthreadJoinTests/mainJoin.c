#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int pj1();
extern int pj2();
extern int pj3();
extern int pj4();
extern int pj5();
extern int pj6();
extern int pj7();
extern int pj8();

extern int pjlast();

extern int pd1();
extern int pd2();
extern int pd3();

extern int psd1();


void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("PthreadJoinDetach:Test %d successful!\n", testnumber);
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
            printf("PthreadJoinDetach:Test %d successful!\n", testnumber);
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

    // rv = pj1();          //pthread_join for function that has already finished 
    // number_of_tests++;
    // check_return_value(1, rv, &successful_tests, "pthread_join for function that has already finished");

    // rv = pj2();          //pthread_join where function is still running
    // number_of_tests++;
    // check_return_value(2, rv, &successful_tests, "pthread_join where function is still running");

    // printf("\npj3 currently disabled because it takes forever\n");
    // rv = pj3();        //starting 2000 threads after each other and join them - takes forever
    // number_of_tests++;
    // check_return_value(3, rv, &successful_tests, "starting 2000 threads after each other and join them - takes forever");

    // rv = pj4();          //try to join the same thread twice
    // number_of_tests++;
    // check_return_value(4, rv, &successful_tests, "try to join the same thread twice");

    // rv = pj5();          //Simple pthread join (very similar to pj2)
    // number_of_tests++;
    // check_return_value(5, rv, &successful_tests, "Simple pthread join (very similar to pj2)");

    // rv = pj6();          //more complex join and cancel test
    // number_of_tests++;
    // check_return_value(6, rv, &successful_tests, "more complex join and cancel test");

    // rv = pj7();          //join void function
    // number_of_tests++;
    // check_return_value(7, rv, &successful_tests, "join void");

    // rv = pj8();          //sanity checks
    // number_of_tests++;
    // check_return_value(8, rv, &successful_tests, "sanity checks");

    // rv = pd1();          //pthread detach should return error if thread cannot be found
    // number_of_tests++; 
    // check_return_value(8, rv, &successful_tests, "pthread detach should return error if thread cannot be found");
    
    // rv = pd2();          //detach pthread on which join is waiting and check if it really isnt joinable
    // number_of_tests++;
    // check_return_value(8, rv, &successful_tests, "detach pthread on which join is waiting and check if it really isnt joinable");
    
    // rv = pd3();         //detach pthread on which join is waiting and check if it really isnt joinable
    // number_of_tests++;
    // check_return_value(8, rv, &successful_tests, "A detached thread should not be joinable");

    rv = psd1();         //TODOs
    number_of_tests++;
    check_return_value(8, rv, &successful_tests, "Todos");




    // printf("-------------------------------------------------------------\n");
    // printf("PthreadJoin:Test last successful!\n");
    // printf("Description: Invalid userspace addresss as value_ptr\n");
    // printf("_____________________________________________________________\n\n");

    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll pthreadJoin() testcases successful\n");
    }
    else
    {
        printf("\n\npthreadJoin() testcases fail\n");
        assert(0);
    }

    // pjlast();          //value_ptr invalild userspace address (should kill process)
    // assert(0);

}

                



                 

                    