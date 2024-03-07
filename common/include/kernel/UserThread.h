#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:                 
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter);
        ~UserThread();
        void Run();

        
    //private:
        UserProcess* process_;

        
};