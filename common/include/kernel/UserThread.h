#pragma once

#include "Thread.h"

class UserThread : public Thread {
public:
    UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type) : Thread(working_dir, name, type) {}
    virtual void Run() override;
};
