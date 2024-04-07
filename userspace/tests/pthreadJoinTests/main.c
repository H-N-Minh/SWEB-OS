#include "stdio.h"

extern int pj1();
extern int pj2();
extern int pj3();
extern int pj4();
extern int pj5();
extern int pj6();
extern int pj7();

int main()
{
    pj1();          //pthread_join for function that has already finished 
    pj2();          //pthread_join where function is still running
    //pj3();        //starting 2000 threads after each other and join them - takes forever
    printf("pj3 currently disabled because it takes forever\n");
    pj4();          //try to join the same thread twice
    pj5();          //Simple pthread join (very similar to pj2)
    pj6();          //more complex join and cancel test
    pj7();          //join void function (not sure what the restriction on that are though)

    printf("\n\npthread_join testcases successful\n");
}

                
                            

                    