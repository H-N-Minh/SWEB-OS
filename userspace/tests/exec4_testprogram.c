#include "stdio.h"
#include "assert.h"
#include "string.h"

//testprogram for exec4 with args 
int main(int argc, char *argv[])
{
   assert(argc == 4);
    //printf("Argc is %d\n", argc);

    char *arguments[] = { "Alle meine", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };
    for(int i = 0; i < argc;  i++)
    {
        //printf("Argument %d is \"%s\"\n", i,  argv[i]);
        assert(strcmp(argv[i], arguments[i]) == 0);
    }

    printf("Exec4 successful!\n");
    return 0;
}