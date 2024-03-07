#pragma once

#include "Thread.h"
#include "Loader.h"

class UserProcess;

class UserThread : public Thread {
public:
    UserThread(FileSystemInfo* working_dir, ustl::string name, Loader* loader, uint32 terminal_number, UserProcess* process);
    virtual ~UserThread();

    void Run() override;

private:
    uint32 terminal_number_;
    UserProcess* process_;
};