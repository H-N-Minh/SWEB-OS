#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

class UserProcess;

class UserThread : public Thread
{
    public:                 
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter);
        ~UserThread();

        bool last_thread_alive_{false};
        bool receieved_cancel_request_{false};




        void Run();

};