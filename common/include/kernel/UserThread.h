#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number, UserProcess* process);

        UserProcess* process_;

        ~UserThread();


};