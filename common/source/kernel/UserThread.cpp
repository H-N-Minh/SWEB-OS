#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"

#include "ArchInterrupts.h"

#include "uvector.h"

// #include "ProcessRegistry.h"
// #include "debug.h"
// #include "PageManager.h"
#include "VfsSyscall.h"
// #include "ustring.h"
// #include "UserProcess.h"
// #include "ArchInterrupts.h"



UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, size_t tid, void* func, void* arg, void* pcreate_helper)
            : Thread(working_dir, name, type, loader), process_(process) , cancel_state_type_lock_("cancel_state_type_lock_")
{
    tid_ = tid;

    debug(TAI_THREAD, "------------------------------------tid value %zu \n", tid);
    debug(TAI_THREAD, "------------------------------------func in userthread is %p\n", func);

    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - tid, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    void* user_stack_ptr = (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * (tid-1));

    if (!func) //for the first thread when we create a process
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                         (void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    }
    else // create the thread for every pthread create
    {
        //pcreate_helper is the wrapper that we need to run first
        //the wrapper will take 2 parameter
        //first one for the function that we want to run
        //second one is the parameter of the function
        ArchThreads::createUserRegisters(user_registers_, (void*) pcreate_helper,
                                         user_stack_ptr,
                                         getKernelStackStartPointer());

        user_registers_->rdi = (size_t)func;
        user_registers_->rsi = (size_t)arg;
        debug(TAI_THREAD, "------------------------------------arg value %zu \n", user_registers_->rdi);
    }


    debug(TAI_THREAD, "----------------------para value %zu \n", (size_t)arg);

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));


    switch_to_userspace_ = 1; 
}

//from Steffi


UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, 
            void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter, bool execv):
            Thread(working_dir, name, type, loader), process_(process)//, join_threads_lock_("join_threads_lock_"), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
            //thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_")
            , cancel_state_type_lock_("cancel_state_type_lock_")
{
    size_t array_offset = 3500;

    if(execv)
    {
        size_t virtual_address =  ArchMemory::getIdentAddressOfPPN(process_->execv_ppn_args_);
        debug(USERTHREAD, "Value of %s.\n", ((char*)virtual_address));


        size_t virtual_page = USER_BREAK / PAGE_SIZE - 1;
        bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page , process_->execv_ppn_args_, 1);
        assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen - in execv");

        for(size_t i = 0; i < process_->exec_argc_; i++)
        {
            *(size_t*)(virtual_address + array_offset + i * sizeof(pointer)) += (USER_BREAK - PAGE_SIZE);
        }
        
    }

    size_t page_for_stack = PageManager::instance()->allocPPN();
    virtual_page_ = USER_BREAK / PAGE_SIZE - thread_counter - 1;
    size_t user_stack_start = USER_BREAK - sizeof(pointer) - PAGE_SIZE * (thread_counter);
    bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page_ , page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen - in thread");
    
    if(wrapper == 0)
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*)user_stack_start, getKernelStackStartPointer());
        debug(USERTHREAD, "First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",user_stack_start, user_stack_start, virtual_page_, virtual_page_);
    }
    else
    {

        ArchThreads::createUserRegisters(user_registers_, (void*)wrapper, (void*)user_stack_start, getKernelStackStartPointer());
        user_registers_->rdi = (size_t)start_routine;
        user_registers_->rsi = (size_t)arg;
        debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",user_stack_start, user_stack_start, virtual_page_, virtual_page_);

    }

    if(execv)
    {
        user_registers_->rdi = process_->exec_argc_;
        user_registers_->rsi = USER_BREAK - PAGE_SIZE + array_offset;
    }

    

    ArchThreads::setAddressSpace(this, loader_->arch_memory_); 

    setTID(thread_counter);

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));


    switch_to_userspace_ = 1; 

}

UserThread::UserThread(UserThread& other, UserProcess* new_process, int32 tid, uint32 terminal_number, Loader* loader)
        : Thread(other, loader), process_(new_process), return_value_(0), finished_(0), joiner_(0), cancel_state_type_lock_("cancel_state_type_lock_")
{
    debug(USERTHREAD, "UserThread COPY-Constructor: start copying from thread (%zu) \n", other.getTID());
    tid_ = tid;

    // copy registers of parent thread, except for RAX (for different fork()-return-value)
    debug(USERTHREAD, "UserThread COPY-Constructor: copying registers from parent to child thread \n");
    ArchThreads::copyUserRegisters(other.user_registers_, user_registers_, getKernelStackStartPointer());
    ArchThreads::setupForkReturnValue(other.user_registers_, user_registers_, new_process->pid_);

    // Setting up AddressSpace and Terminal
    debug(USERTHREAD, "UserThread COPY-Constructor: setting up Child with its own CR3\n");
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;
}






// TODO: these 2 methods need locking for thread safe
void UserThread::setReturnValue(void* return_value)
{
    return_value_ = return_value;
    finished_ = 1;
    if(joiner_)
    {
        debug(USERTHREAD, "UserThread::setReturnValue: Worker (%zu) waking up Joiner (%zu)\n",
                            getTID(), joiner_->getTID());
        Scheduler::instance()->wake(joiner_);
    }
    debug(USERTHREAD, "UserThread::setReturnValue: worker (%zu) finished, return value: %zu. Going to sleep till Joined\n",
                        getTID(), (size_t) return_value);
    Scheduler::instance()->sleep();

    assert(!finished_ && "Worker should only wakes up after Joined, then kill itself\n");
    kill();
}

void UserThread::getReturnValue(void** return_value, UserThread* worker)
{
    if(!finished_)
    {
        joiner_ = (UserThread*) currentThread;
        debug(USERTHREAD, "UserThread::getReturnValue: Worker (%zu) is not finished, Joiner (%zu) going to sleep\n",
                            getTID(), currentThread->getTID());
        Scheduler::instance()->sleep();

        assert(finished_ && "Joiner should wakes up only when the result is finished");
    }
    *return_value = return_value_;
    finished_ = 0;  // reset the finished flag so the worker thread can wake up and kill itself
    if (worker->getState() == ThreadState::Sleeping)
    {
        debug(USERTHREAD, "UserThread::getReturnValue: Joiner (%zu) waking up Worker (%zu)\n",
                            currentThread->getTID(), worker->getTID());
        Scheduler::instance()->wake(worker);
    }
}


// DO NOT use new / delete in this Method, as it is sometimes called from an Interrupt Handler with Interrupts disabled
// void UserThread::kill()
// {
//   debug(THREAD, "kill: Called by <%s (%zu)>. Preparing Thread <%s (%zu)> for destruction\n", currentThread->getName(),
//         currentThread->getTID(), getName(), this->getTID());

//   // remove itself from list of threads in process
//   if (((UserThread*) currentThread)->process_)
//   {
//     UserProcess* process = ((UserThread*) currentThread)->process_;
//     auto thread = ustl::find(process->threads_.begin(), process->threads_.end(), currentThread);
//     if (thread != process->threads_.end()) {
//         process->threads_.erase(thread);
//     }
//   }

//   // exactly like original kill()
//   setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

//   if (currentThread == this)
//   {
//     ArchInterrupts::enableInterrupts();
//     Scheduler::instance()->yield();
//     assert(false && "This should never happen, how are we still alive?");
//   }
// }

UserThread::~UserThread()
{
    debug(USERTHREAD, "Thread with id %ld gets destroyed.\n", getTID());

    if(last_thread_alive_)
    {
        debug(USERTHREAD, "Userprocess gets destroyed by thread with id %ld.\n", getTID());
        delete process_;
        process_ = 0;
    }

    if(unlikely(last_thread_before_exec_))
    {
        debug(USERTHREAD, "Last thread %ld before exec get destroyed.\n", getTID());
        assert(Scheduler::instance()->isCurrentlyCleaningUp());
        delete loader_;
        process_->loader_ = process_->execv_loader_;
        process_->execv_loader_ = 0;

        if (process_->fd_ > 0)
        {
            VfsSyscall::close(process_->fd_);
        }
        process_->fd_ = process_->execv_fd_;

        assert(process_->threads_.size() == 0);
        process_->thread_counter_++;
        UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_, process_->loader_, process_, 0, 0, 0, process_->thread_counter_, true);
        process_->threads_.push_back(new_thread);
        Scheduler::instance()->addNewThread(new_thread);
    }
}

//from Stefanie
void UserThread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

    assert(currentThread == this);

    // this->process_->threads_lock_.acquire();
    // send_kill_notification();
    // this->process_->threads_lock_.release();

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "This should never happen, how are we still alive?");
  }
}

// void UserThread::Run(){
//     assert(0);
// }


//  void UserThread::send_kill_notification()
//  {
//   UserThread& currentUserThread = *((UserThread*)currentThread);
//   UserProcess& current_process = *currentUserThread.process_;

//   currentUserThread.join_threads_lock_.acquire();
//   for(UserThread* join_thread : currentUserThread.join_threads_)
//   {
//     if(!current_process.check_if_thread_in_threadList(join_thread))
//     {
//       currentUserThread.join_threads_lock_.release();
//       return;
//     }
//     join_thread->thread_gets_killed_lock_.acquire();
//     join_thread->thread_killed = true;
//     join_thread->thread_gets_killed_.signal();
//     join_thread->thread_gets_killed_lock_.release();
//   }
//   currentUserThread.join_threads_lock_.release();
//  }
