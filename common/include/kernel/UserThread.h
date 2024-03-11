#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* workingDir, const ustl::string& filename, Thread::TYPE type, uint32 terminalNumber, Loader* loader, UserProcess* process, size_t tid, void* startRoutine, void* arg);
        ~UserThread();
        UserProcess* process_;
        void Run();
};
