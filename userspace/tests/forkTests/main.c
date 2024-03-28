#include "unistd.h"
extern int fork1();
extern int fork2();

int main()
{
    int pid = fork1();

    sleep(1);

    if(pid!=0)
    {
        int pid = fork2();  //TODOs 
    }
        
    
      

}