#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"
#include "UserProcess.h"

class UserProcess;


class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, size_t tid, void* func, void* para, void* pcreate_helper, bool execv = false);

        // COPY CONSTRUCTOR
        UserThread(UserThread& other, UserProcess* process, int32 tid, uint32 terminal_number, Loader* loader);

        ~UserThread();
        UserProcess* process_{0};
        void Run(){}

        void kill() override;

        // FOR PTHREAD_JOIN//////////////////////////////
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
        // FOR PTHREAD_JOIN//////////////////////////////



        bool last_thread_alive_{false};
        bool last_thread_before_exec_{false};


        bool wants_to_be_canceled_{false};

        size_t virtual_page_;

        bool exit_send_cancelation_{false};

        // Mutex join_threads_lock_;
        ustl::vector<UserThread*> join_threads_;                //TODO:needs to be cleaned somewhere
        bool thread_killed{false};             //not the best naming: TODO
        // Mutex thread_gets_killed_lock_;
        // Condition thread_gets_killed_;

        bool canceled_thread_wants_to_be_killed_{false};   

        Mutex cancel_state_type_lock_;
        // CANCEL_STATE cancel_state_{CANCEL_STATE::PTHREAD_CANCEL_ENABLE};  //currently not used
        // CANCEL_TYPE cancel_type_{CANCEL_TYPE::PTHREAD_CANCEL_DEFERRED};    //currently not used
        bool to_late_for_cancel_{false};

        void send_kill_notification();

};