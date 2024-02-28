#pragma once

#include "Thread.h"
#include "kprintf.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number, UserProcess* process);

        UserProcess* process_;

        ~UserThread();

        virtual void Run()
        {
            debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n"); //maybe not userprocess and maybe even wrong here
            assert(false);
        };
};