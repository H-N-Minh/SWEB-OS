#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, 
                    Loader* loader, UserProcess* process, int32 tid);
        ~UserThread();
        UserProcess* process_;
        void Run();
    
    private:
        int32 tid_;
};
