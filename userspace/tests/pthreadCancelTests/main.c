#include "stdio.h"

extern int pca1();
extern int pca2();
extern int pca3();
extern int pca4();
extern int pca5();

extern int sc1();
extern int sc2();

int main()
{
    pca1();     //Cancel running thread
    pca2();     //Try to cancel already dead thread
    pca3();     //No cancelation without cancelation point
    pca4();     //Async cancelation should cancel thread without cancelation point
    pca5();     //simple pthread_cancel of dead thread with join afterwards

    printf("\n");

    sc1();     //Check if setting cancel type and state works
    sc2();     //pretty similar to sc1

    printf("\n\npthread_cancel testcases successful\n");


}

//Todos:  pca4(Test can also be used, that is dont terminate for deffered type)
