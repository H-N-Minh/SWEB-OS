#pragma once

#include "Thread.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "Console.h"

class UserThread : public Thread {
public:
    UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number);
    virtual void Run() override;
};
