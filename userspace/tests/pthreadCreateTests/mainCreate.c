#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int pc1();
extern int pc2();
extern int pc3();
extern int pc4();
extern int pc5();
extern int pc6();
extern int pc7();
extern int pc8();

extern int pclast();

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

int main()
{
    int successful_tests = 0;
    int number_of_tests = 0;
    int rv;
    

    rv = pc1();  //sanity checks
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "sanity checks");

    rv = pc2();  //simple pthread_create and check if thread id gets set
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "simple pthread_create and check if thread id gets set");

    rv = pc3();  //starting 250 threads
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "starting 250 threads");

    rv = pc4();  //check if to running threads have different id
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "check if to running threads have different id");

    rv = pc5();  //pthread create with argument
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "pthread create with argument");

    rv = pc6();  //running 250 simultaniously
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "running 250 simultaniously");

    rv = pc7();  //calling pthread_create inside pthread_create inside pthread_create
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "rcalling pthread_create inside pthread_create inside pthread_create");

    
    rv = pc8(); //testing pthread_create attributes (detach):
    //pthread create with wrong attributes
    //using pthread attributes twice for different threads
    //destroying twice
    //using a destroyed attribute
    //destroy attribute with address not in userspace
    //init attribute with address not in userspace
    //detachstate after init should be joinable
    //setting pthread_attribute to detach
    //join and cancel if state is detached
    //set and get if attributes is not initalized      
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "testing pthread_create attributes (detach)");



    printf("-------------------------------------------------------------\n");
    printf("PthreadChreate:Test last successful! (if no assertion)\n");
    printf("Description: invalid userspace addresss as thread_id\n");
    printf("_____________________________________________________________\n\n");

    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll pthreadCreate() testcases successful\n");
    }
    else
    {
        printf("\n\npthreadCreate() testcases fail\n");
        assert(0);
    }

    rv = pclast();  //invalid userspace addresss as thread_id (should kill process but no assertion)
    assert(0);

}
                    