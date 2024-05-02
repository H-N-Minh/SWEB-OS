#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int fdTestSimple();
extern int fdTest();

extern int forklfds();
extern int forklfds2();
extern int forklfds3();

extern int forklfds_debugmode();

void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("LocalFD:Test %d successful!\n", testnumber);
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
            printf("LocalFD:Test %d fail!\n", testnumber);
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

    //Checks if simple read and write works with local fd
    rv = fdTestSimple();
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Checks if simple read and write works with local fd");      

    //Open and close more local file descriptor and read or write to them    
    rv = fdTest();
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Open and close more local fdsc and read or write to them");      

    //Checks if local filedescriptors gets copied and child can read them and if parent can still read/write after child closed descriptor
    rv = forklfds();    
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Lfds copied? child read? parent read after child closed fds?");     
    sleep(1);

    // Check if both parent and child can use local file descriptor after fork;
    rv = forklfds2(); 
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "Check if both parent and child can use local fdsc after fork");      
    sleep(1);

    //Checks if parent can read the data that child wrote to file before
    rv = forklfds3(); 
    number_of_tests++;
    check_return_value(number_of_tests, rv, &successful_tests, "parent can read the data that child wrote to file before?");   


    if(successful_tests == number_of_tests)
    {
        printf("\n\nAll Localfds tests() testcases successful\n");
    }
    else
    {
        printf("\n\nLocalfds tests() testcases fail\n");
        assert(0);
    }

}
    