#pragma once

#include <types.h>

#include "Thread.h"

class Syscall
{
  public:
    static size_t syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

    static void exit(size_t exit_code);
    static void outline(size_t port, pointer text);

    static size_t write(size_t fd, pointer buffer, size_t size);
    static size_t read(size_t fd, pointer buffer, size_t count);
    static size_t close(size_t fd);
    static size_t open(size_t path, size_t flags);
    static void pseudols(const char *pathname, char *buffer, size_t size);

    static size_t createprocess(size_t path, size_t sleep);
    static void trace();

    static uint32 get_thread_count();

    static int pthread_create(size_t* thread, size_t* attr, void *(*start_routine)(void*), void* arg);
    static int pthread_cancel(size_t* thread_id);

    static int pthread_setcancelstate(CancelState state, CancelState* oldstate);
    static int pthread_setcanceltype(CancelType type, CancelType* oldtype);
};

