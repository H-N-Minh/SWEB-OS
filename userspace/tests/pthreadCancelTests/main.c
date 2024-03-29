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
    pca3();     
    pca4();  
    //pca5();  

    printf("\n");

    sc1();  
    sc2(); 

    printf("\n\npthread_cancel testcases successful\n");


}
