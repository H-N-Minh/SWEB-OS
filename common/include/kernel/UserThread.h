#pragma once

#include "Thread.h"

class UserProcess;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, size_t tid, void* func, void* para, void* pcreate_helper); //thread for pthread_create

        UserThread(UserThread& other, UserProcess* process, int32 tid, uint32 terminal_number, Loader* loader);

        ~UserThread();
        UserProcess* process_;
        void Run() override;

        void kill() override; //from Minh

        void* return_value_;
        uint32 finished_;
        UserThread* joiner_;
        bool can_be_canceled_ = false;

        /**
         * set the return value after thread finished, wake up joiner-thread if necessary
        */
        void setReturnValue(void* return_value);

        /**
         * retrieve the return value of a thread, if it is not finished, the joiner will be stored and then sleep
        */
        void getReturnValue(void** return_value, UserThread* joiner);


};