#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                   Loader* loader, UserProcess* process, size_t tid); //first thread
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, size_t tid, void *(*func)(void*), void* arg); //thread for pthread_create
        ~UserThread();
        UserProcess* process_;
        void Run();
};