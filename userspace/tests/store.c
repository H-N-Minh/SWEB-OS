//
// Created by fabian on 06.03.24.
//
#include "../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int main()
{

    char * string_string_to_store = " Hello, World/n";
    size_t id = 0;

    __syscall(sc_string_store, string_string_to_store, id, 0x0, 0x0, 0x0);

    assert(ret==0);

   //f(ret != 0){
   //   printf("Error in store \n");
   //   return -1;
   //
    assert(ret==1);
    return 0;
}