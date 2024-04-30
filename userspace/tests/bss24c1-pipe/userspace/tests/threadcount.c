/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
int main(int argc, char *argv[]) {
    printf("Threadcount: %d\n", get_thread_count());
    return 0;
}
