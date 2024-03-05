#pragma once

#include "Thread.h"

#define STACK_CANARY ((uint32)0xDEADDEAD ^ (uint32)(size_t)this)

class UserThread : public Thread
{
    public:


    /**
     * Constructor for a new thread with a given working directory, name and type
     * @param working_dir working directory information for the new Thread
     * @param name The name of the thread
     */
    UserThread(FileSystemInfo* working_dir, ustl::string name);

    virtual ~UserThread();


    /**
     * runs whatever the user wants it to run;
     */
    virtual void Run() = 0;



    ArchThreadRegisters* user_registers_;

    uint32 switch_to_userspace_;

    Loader* loader_;


private:
    UserThread(Thread const &src);
    UserThread &operator=(Thread const &src);

    volatile ThreadState state_;

    size_t tid_;

    Terminal* my_terminal_;

protected:
    ThreadState getState() const;

    FileSystemInfo* working_dir_;

    ustl::string name_;
};