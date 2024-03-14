#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

class UserProcess;

class UserThread : public Thread
{
    public: 
        enum CANCEL_STATE {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
        enum CANCEL_TYPE {PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_ASYNCHRONOUS};

        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter);
        ~UserThread();

        bool last_thread_alive_{false};
        bool last_thread_before_exec_{false};


        bool wants_to_be_canceled_{false};

        size_t virtual_page_;

        UserThread::CANCEL_STATE cancel_state_{CANCEL_STATE::PTHREAD_CANCEL_ENABLE};  //currently not used
        UserThread::CANCEL_TYPE cancel_type_{CANCEL_TYPE::PTHREAD_CANCEL_DEFERRED};


        void Run();

};