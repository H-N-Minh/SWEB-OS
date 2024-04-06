#pragma once

#include "types.h"
#include "fs/FileSystemInfo.h"
#include "debug.h"
#include "Condition.h"
#include "Mutex.h"
#include "uvector.h"

#define STACK_CANARY ((uint32)0xDEADDEAD ^ (uint32)(size_t)this)

enum ThreadState{Running, Sleeping, ToBeDestroyed};
enum SystemState {BOOTING, RUNNING, KPANIC};

enum CancelState {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};

// PTHREAD_CANCEL_EXIT: similar to PTHREAD_CANCEL_ASYNCHRONOUS, except thread gets canceled no matter what CancelState is.
enum CancelType {PTHREAD_CANCEL_DEFERRED = 2, PTHREAD_CANCEL_ASYNCHRONOUS = 3, PTHREAD_CANCEL_EXIT=4};

extern SystemState system_state;

class UserProcess;
class Thread;
class ArchThreadRegisters;
class Loader;
class Terminal;
class Mutex;
class Lock;

extern Thread* currentThread;

class Thread
{
    friend class Scheduler;
    public:

        static const char* threadStatePrintable[3];

        enum TYPE { KERNEL_THREAD, USER_THREAD };

        /**
         * Constructor for a new thread with a given working directory, name and type
         * @param working_dir working directory information for the new Thread
         * @param name The name of the thread
         * @param type The type of the thread (user or kernel thread)
         */
        Thread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader=0,  UserProcess* parent_process = nullptr);

        /**
         * Copy constructor, used for fork()
         */
        Thread(Thread &src, Loader* loader, UserProcess* parent_process = nullptr);

        virtual ~Thread();
        void setTID(size_t tid);


        /**
         * Marks the thread to be deleted by the scheduler.
         * DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
         */
        virtual void kill();

        /**
         * runs whatever the user wants it to run;
         */
        virtual void Run() = 0;

        void* getKernelStackStartPointer();

        bool isStackCanaryOK();

        static bool currentThreadIsStackCanaryOK();

        const char* getName();

        size_t getTID() const;

        Terminal* getTerminal();

        void setTerminal(Terminal *my_term);

        FileSystemInfo* getWorkingDirInfo();

        void setWorkingDirInfo(FileSystemInfo* working_dir);

        /**
         * Prints a backtrace (i.e. the call stack) to the debug output.
         * @param use_stored_thread_info determines whether to use the stored or the current thread registers
         */
        void printBacktrace();
        void printBacktrace(bool use_stored_registers);

        /**
         * Tells the scheduler if this thread is ready for scheduling
         * @return true if ready for scheduling
         */
        bool schedulable();


        uint32 kernel_stack_[2048]{};
        ArchThreadRegisters* kernel_registers_;
        ArchThreadRegisters* user_registers_;

        uint32 switch_to_userspace_;

        Loader* loader_;


        void setState(ThreadState state);

        //ThreadState getState() const;

        /**
         * A part of the single-chained waiters list for the locks.
         * It references to the next element of the list.
         * In case of a spinlock it is a busy-waiter, else usually it is a sleeper ^^.
         */
        Thread* next_thread_in_lock_waiters_list_;

        /**
         * The information which lock the thread is currently waiting on.
         */
        Lock* lock_waiting_on_;

        /**
         * A single chained list containing all the locks held by the thread at the moment.
         * This list is not locked. It may only be accessed by the thread himself,
         * or by other threads in case they can ENSURE that this thread is not able to run at this moment.
         * Changing the list has to be done atomic, else it cannot be ensured that the list is valid at any moment!
         */
        Lock* holding_lock_list_;

        void cancelThread();

        void setCancelState(CancelState state);
        CancelState getCancelState() const;

        void setCancelType(CancelType type);
        CancelType getCancelType() const;

        UserProcess* getUserProcess() const {
          return parent_process_;
        }

    private:
        Thread(Thread const &src);
        Thread &operator=(Thread const &src);

        volatile ThreadState state_;
        UserProcess* parent_process_;
    public:
        CancelState cancel_state_{CancelState::PTHREAD_CANCEL_ENABLE};  //default cancel state is ENABLED
        CancelType cancel_type_{CancelType::PTHREAD_CANCEL_DEFERRED}; //default cancel type is DEFERRED


    protected:
        size_t tid_;
        Terminal* my_terminal_;

        ThreadState getState() const;

        FileSystemInfo* working_dir_;

        ustl::string name_;

  public:

    Thread::TYPE type_;

};

