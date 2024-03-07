#pragma once

#include "Thread.h"
#include "Loader.h"

class UserThread : public Thread {
public:
    UserThread(FileSystemInfo* working_dir, ustl::string name, Loader* loader);
    virtual ~UserThread();

    void Run() override;

private:
    Loader* loader_;
};