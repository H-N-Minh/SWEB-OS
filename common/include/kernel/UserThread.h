#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

class UserProcess;

#define META_SIZE 6     // If this is changed then the guards location in pthread.c in userspace must be updated as well

// PTHREAD_CANCEL_EXIT: similar to PTHREAD_CANCEL_ASYNCHRONOUS, except thread gets canceled no matter what CancelState is.
enum CancelType {PTHREAD_CANCEL_DEFERRED = 2, PTHREAD_CANCEL_ASYNCHRONOUS = 3, PTHREAD_CANCEL_EXIT=4};
enum CancelState {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
enum JoinState {PTHREAD_CREATE_DETACHED = 5, PTHREAD_CREATE_JOINABLE = 4, PCD_TO_BE_JOINED = 6, PCJ_TO_BE_JOINED = 7};

typedef struct {
    JoinState detach_state;  // Detach state: PTHREAD_CREATE_JOINABLE or PTHREAD_CREATE_DETACHED
    void *stack_addr;  // Stack address
    size_t stack_size; // Stack size
    int priority;      // Thread priority
    int initialized;
} pthread_attr_t;

class UserThread : public Thread
{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                    Loader* loader, UserProcess* process, void* func, void* para, void* pcreate_helper);

        UserThread(UserThread& other, UserProcess* process); // COPY CONSTRUCTOR

        ~UserThread();
        
        void Run(){}
        void kill() override;
        void send_kill_notification();
        bool schedulable() override;

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg, pthread_attr_t* attr);
        int cancelThread(size_t thread_id);
        int joinThread(size_t thread_id, void**value_ptr);
        void exitThread(void* value_ptr);
        int detachThread(size_t thread_id);

        /**
         * @brief setup the header for a thread stack including guards, metadata for userspace locks
         * @return the top of the user stack, which is right under the meta header
        */
        size_t setupMetaHeader();


        UserProcess* process_;
        size_t vpn_stack_;
        size_t user_stack_ptr_;            //Todos: copy in fork
        size_t page_for_stack_;

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
        size_t cond_flag_{NULL};         // pointer to boolean in user stack, indicate whether the thread is schedulable or not
        size_t mutex_flag_{NULL};
        // the Meta data is ordered as follows:
                    // 1. Guard
                    // 2. Mutex flag
                    // 3. Mutex waiter list 
                    // 4. Cond flag
                    // 5. Cond waiter list
                    // 6. Guard
                    // 7. user_stack_ptr (the user stack starts from here)
        
        // pointer of the top of 1st page of the stack
        size_t top_stack_;
        

};
