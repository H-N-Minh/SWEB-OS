#include "stdio.h" //
#include "assert.h"
#include "string.h"

//testprogram for exec4 with args 
int main(int argc, char *argv[])
{
    if(argc == 0)
    {
        printf("Exec5 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "3") == 0)
    {
        assert(argc == 302);
        //printf("Argc is %d\n", argc);

        char *arguments[] = { 
        "3","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","2",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
        "1","1","1","1","1","1","1","1","1","1",
        "1","1", (char *)0 };

        for(int i = 0; i < argc;  i++)
        {
        //printf("Argument %d is \"%s\"\n", i,  argv[i]);
        assert(strcmp(argv[i], arguments[i]) == 0);
        }

        printf("Exec3 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "4") == 0)
    {
        assert(argc == 4);
        //printf("Argc is %d\n", argc);

        char *arguments[] = { "4", "Alle meine Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };
        for(int i = 0; i < argc;  i++)
        {
            //printf("Argument %d is \"%s\"\n", i,  argv[i]);
            assert(strcmp(argv[i], arguments[i]) == 0);
        }

        printf("Exec4 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "6") == 0)
    {
        assert(argc == 4);
        //printf("Argc is %d\n", argc);

        char *arguments[] = { "6", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };
        for(int i = 0; i < argc;  i++)
        {
            //printf("Argument %d is \"%s\"\n", i,  argv[i]);
            assert(strcmp(argv[i], arguments[i]) == 0);
        }

        printf("Exec6 successful!\n");
        sleep(1);
        return 0;
    }
    else if(strcmp(argv[0], "7") == 0)
    {
        printf("Exec7 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "8") == 0)
    {
        assert(argc == 7);
        char* long_word = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj";
    
        assert(strcmp(argv[1], long_word) == 0);
        assert(strcmp(argv[2], long_word) == 0);
        assert(strcmp(argv[3], long_word) == 0);
        assert(strcmp(argv[4], long_word) == 0);
        assert(strcmp(argv[5], long_word) == 0);
        assert(strcmp(argv[6], long_word) == 0);

        printf("Exec8 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "9") == 0)
    {
        assert(argc == 3);
        char *arguments[] = { "9", "slkdfjlsd", "lskdfj", (char *)0 };
        for(int i = 0; i < argc;  i++)
        {
            //printf("Argument %d is \"%s\"\n", i,  argv[i]);
            assert(strcmp(argv[i], arguments[i]) == 0);
        }
        printf("Exec9 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "10") == 0)
    {
        assert(argc == 6);
        char* long_word = "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj"
                      "aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjjaaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggghhhhhhhhhhiiiiiiiiiijjjjjjjjjj";
    
        assert(strcmp(argv[1], long_word) == 0);
        assert(strcmp(argv[2], long_word) == 0);
        assert(strcmp(argv[3], long_word) == 0);
        assert(strcmp(argv[4], long_word) == 0);
        assert(strcmp(argv[5], long_word) == 0);

        printf("Exec10 successful!\n");
        return 0;
    }
    else if(strcmp(argv[0], "11") == 0)
    {
        assert(argc == 3);
        //printf("Argc is %d\n", argc);

        if(strcmp(argv[1], "1") == 0)
        {
            char *arguments[] = { "11", "1", "abc", (char *)0 };
            for(int i = 0; i < argc;  i++)
            {
                assert(strcmp(argv[i], arguments[i]) == 0);
            }
            printf("Exec11_1 successful!\n");
        }

        if(strcmp(argv[1], "2") == 0)
        {
            char *arguments2[] = { "11", "2", "cde", (char *)0 };
            for(int i = 0; i < argc;  i++)
            {
                assert(strcmp(argv[i], arguments2[i]) == 0);
            }
            printf("Exec11_2 successful!\n");
        }
        return 0;
    }
    assert(0);
}


