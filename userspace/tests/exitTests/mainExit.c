#include "stdio.h"
#include "assert.h"

#define PRINT_DESCRIPTION 1

extern int e1();
extern int e2();


void check_return_value(int testnumber, int rv, int* successful_tests, char* description)
{
    if(rv == 0)
    {
        if(PRINT_DESCRIPTION)
        {
            printf("-------------------------------------------------------------\n");
            printf("Exit:Test %d successful! (check if no assertion and if threads are dead)\n", testnumber);
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
            printf("Exit:Test %d fail! \n", testnumber);
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

  rv = e1();          
  number_of_tests++;
  check_return_value(number_of_tests, rv, &successful_tests, "e1 successful if no assertion gets raised!");

  rv = e2();          
  number_of_tests++;
  check_return_value(number_of_tests, rv, &successful_tests, "100 times exit at the same time");


  if(successful_tests == number_of_tests)
  {
    printf("\n\nAll Exit testcases successful (Check if threads are dead and no assertion)\n");
  }
  else
  {
    printf("\n\nExit testcases fail\n");
    assert(0);
  }
  return 0;
}  
