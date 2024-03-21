#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"
#include "UserProcess.h"

class UserProcess;

enum CANCEL_STATE {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
enum CANCEL_TYPE {PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_ASYNCHRONOUS, PTHREAD_CANCEL_EXIT};

class UserThread : public Thread
{
    public: 


        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter, bool execv);
        UserThread(UserThread const &src, UserProcess* process = NULL, size_t thread_counter = 0);
        ~UserThread();

        UserProcess* process_{0};

        bool last_thread_alive_{false};
        bool last_thread_before_exec_{false};


        bool wants_to_be_canceled_{false};

        size_t virtual_page_;

        bool exit_send_cancelation_{false};


        UserThread* join_thread_{NULL};
        bool thread_killed{false};             //not the best naming: TODO
        Mutex thread_gets_killed_lock_;
        Condition thread_gets_killed_;



        bool canceled_thread_wants_to_be_killed_{false};   
        
        ustl::vector<UserThread*> cancel_threads_;                //TODO:needs to be cleaned somewhere

        Mutex cancel_state_type_lock_;
        CANCEL_STATE cancel_state_{CANCEL_STATE::PTHREAD_CANCEL_ENABLE};  //currently not used
        CANCEL_TYPE cancel_type_{CANCEL_TYPE::PTHREAD_CANCEL_DEFERRED};

        bool recieved_pthread_exit_notification_{false};
        Mutex has_recieved_pthread_exit_notification_lock_;
        Condition has_recieved_pthread_exit_notification_;


        void Run();
        void kill();

};