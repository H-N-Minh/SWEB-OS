#include "stdio.h"
#include "unistd.h"

extern int fdTest();
extern int fdTestSimple();
//extern int pipeTest();

int main()
{
    printf("------------------------------------------------------------\n");
    printf("FD_Test_Simple\n");
    fdTestSimple();
    sleep(2);
    printf("------------------------------------------------------------\n");
    printf("FD_Test\n");
    fdTest();    
    sleep(2);
    printf("------------------------------------------------------------\n");
//    printf("Pipe_Test\n");
//    pipeTest();
//    sleep(2);


    printf("\n\nall local fd tests successfull\n");
}
    