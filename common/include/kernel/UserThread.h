#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"
#include "UserProcess.h"

class UserProcess;



// PTHREAD_CANCEL_EXIT: similar to PTHREAD_CANCEL_ASYNCHRONOUS, except thread gets canceled no matter what CancelState is.
enum CancelType {PTHREAD_CANCEL_DEFERRED = 2, PTHREAD_CANCEL_ASYNCHRONOUS = 3, PTHREAD_CANCEL_EXIT=4};
enum CancelState {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
enum JoinState {PTHREAD_CREATE_DETACHED, PTHREAD_CREATE_JOINABLE, PCD_TO_BE_JOINED, PCJ_TO_BE_JOINED};

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, void* func, void* para, void* pcreate_helper, bool execv = false);

        UserThread(UserThread& other, UserProcess* process); // COPY CONSTRUCTOR

        ~UserThread();
        
        void Run(){}
        void kill() override;
        void send_kill_notification();
        bool schedulable() override;

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg);
        int cancelThread(size_t thread_id);
        int joinThread(size_t thread_id, void**value_ptr);
        void exitThread(void* value_ptr);
        int detachThread(size_t thread_id);

        
        UserProcess* process_;
        size_t vpn_stack_;

        //exit
        bool last_thread_alive_{false};

        //exec
        bool last_thread_before_exec_{false};

        //pthread join (join_threads_ are locked by threads_lock_)
        ustl::vector<UserThread*> join_threads_; 
        bool thread_killed{false};
        Mutex thread_gets_killed_lock_;                                //Locking order: x
        Condition thread_gets_killed_;

        Mutex join_state_lock_;
        JoinState join_state_{JoinState::PTHREAD_CREATE_JOINABLE};


        //pthread_cancel
        bool wants_to_be_canceled_{false};
        Mutex cancel_state_type_lock_;                                  //Locking order: x
        CancelState cancel_state_{CancelState::PTHREAD_CANCEL_ENABLE};
        CancelType cancel_type_{CancelType::PTHREAD_CANCEL_DEFERRED};

        //userspace locks
        size_t request_to_sleep_{NULL};         // pointer to boolean in user stack, indicate whether the thread is schedulable or not

        size_t waiting_list_ptr_{NULL};
        size_t currently_waiting_ptr_{NULL};
 

};
