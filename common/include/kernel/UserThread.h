#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, 
                    Loader* loader, UserProcess* process, int32 tid, void* func, void* para, void* pcreate_helper);
        ~UserThread();
        UserProcess* process_;
        void Run();
};
