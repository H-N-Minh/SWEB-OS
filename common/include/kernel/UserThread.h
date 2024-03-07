#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:                 
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter);
        ~UserThread();
        UserProcess* process_;
        inline static size_t TID_counter_{1};
        void Run();
        
};