#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, size_t tid, void* func, void* para, void* pcreate_helper); //thread for pthread_create
        ~UserThread();
        UserProcess* process_;
        void Run();

        void* return_value_;
        uint32 finished_;
        UserThread* joiner_;

        void setReturnValue(void* return_value);
};