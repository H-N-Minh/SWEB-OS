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
#include "VfsSyscall.h"

// TODO: explain calculation related to execv()
UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, void* func, void* arg, void* pcreate_helper, bool execv)
            : Thread(working_dir, name, type, loader), process_(process) ,
                thread_gets_killed_lock_("thread_gets_killed_lock_"), 
                thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"), cancel_state_type_lock_("cancel_state_type_lock_")
{
    tid_ = ArchThreads::atomic_add(UserProcess::tid_counter_, 1);
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
    vpn_stack_ = USER_BREAK / PAGE_SIZE - tid_ - 1;
    bool vpn_mapped = loader_->arch_memory_.mapPage(vpn_stack_, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    // sizeof(pointer)*4 because 1 is default and 3 spots are reserved for userspace locks
    size_t user_stack_ptr = (USER_BREAK - sizeof(pointer)*4 - PAGE_SIZE * tid_);
    request_to_sleep_ = user_stack_ptr + sizeof(pointer);

    if (!func) //for the first thread when we create a process
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                         (void*) user_stack_ptr, getKernelStackStartPointer());

        debug(USERTHREAD, "Create First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
               user_stack_ptr, user_stack_ptr, vpn_stack_, vpn_stack_);
    }
    else // create the thread for every pthread create
    {
        ArchThreads::createUserRegisters(user_registers_, (void*) pcreate_helper, (void*) user_stack_ptr,
                                         getKernelStackStartPointer());

        user_registers_->rdi = (size_t)func;
        user_registers_->rsi = (size_t)arg;
        user_registers_->rdx = (size_t) user_stack_ptr + sizeof(pointer)*3; // address of the top of stack, relevant for userspace locks
        debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
                user_stack_ptr, user_stack_ptr, vpn_stack_, vpn_stack_);
    }

    if(execv)
    {
        user_registers_->rdi = process_->exec_argc_;
        user_registers_->rsi = USER_BREAK - PAGE_SIZE + array_offset;
    }

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));


    switch_to_userspace_ = 1; 
}


UserThread::UserThread(UserThread& other, UserProcess* new_process)
            : Thread(other, new_process->loader_), process_(new_process), vpn_stack_(other.vpn_stack_),
            thread_gets_killed_lock_("thread_gets_killed_lock_"),  thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"),
            cancel_state_type_lock_("cancel_state_type_lock_"), cancel_state_(other.cancel_state_), cancel_type_(other.cancel_type_),
            request_to_sleep_(other.request_to_sleep_)
{
    debug(FORK, "UserThread COPY-Constructor: start copying from thread (TID:%zu) \n", other.getTID());
    tid_ =  ArchThreads::atomic_add(UserProcess::tid_counter_, 1);
    working_dir_ = new_process->working_dir_;

    // copy registers of parent thread, except for RAX (for different fork()-return-value)
    debug(USERTHREAD, "UserThread COPY-Constructor: copying registers from parent to child thread \n");
    ArchThreads::copyUserRegisters(other.user_registers_, user_registers_, getKernelStackStartPointer(), &new_process->loader_->arch_memory_);
    ArchThreads::setupForkReturnValue(other.user_registers_, user_registers_, new_process->pid_);

    // Setting up AddressSpace and Terminal
    debug(USERTHREAD, "UserThread COPY-Constructor: setting up Child with its own CR3\n");
    ArchThreads::setAddressSpace(this, new_process->loader_->arch_memory_);
    if (main_console->getTerminal(new_process->terminal_number_))
        setTerminal(main_console->getTerminal(new_process->terminal_number_));

    switch_to_userspace_ = 1;
}

UserThread::~UserThread()
{
    debug(USERTHREAD, "Thread with id %ld gets destroyed.\n", getTID());

    //assert(join_threads_.size() == 0 && "There are still waiting threads to get joined, but this is last thread"); //TODO

    if(last_thread_alive_)
    {
        assert(process_->threads_.size() == 0 && "Not all threads removed from threads_");
       // assert(process_->thread_retval_map_.size() == 0 && "There are still values in retval map");
        debug(USERTHREAD, "Userprocess gets destroyed by thread with id %ld.\n", getTID());
        delete process_;
        process_ = 0;
    }

    if(unlikely(last_thread_before_exec_))
    {
        assert(process_->threads_.size() == 0 && "Not all threads removed from threads_");    //TODO: also check 
        //assert(process_->thread_retval_map_.size() == 0 && "There are still values in retval map");

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

        UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_, process_->loader_, process_,0, 0, 0, true);
        process_->threads_.push_back(new_thread);
        Scheduler::instance()->addNewThread(new_thread);
    }
}


void UserThread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

    assert(currentThread == this && "Only the thread itself can kill itself\n");

    // FOR PTHREAD JOIN
    process_->threads_lock_.acquire();
    send_kill_notification();
    process_->threads_lock_.release();

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "This should never happen, how are we still alive?");
  }
}

 void UserThread::send_kill_notification()
 {
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  for(UserThread* join_thread : currentUserThread.join_threads_)
  {
    if(current_process.isThreadInVector(join_thread))
    {
        join_thread->thread_gets_killed_lock_.acquire();
        join_thread->thread_killed = true;
        join_thread->thread_gets_killed_.signal();
        join_thread->thread_gets_killed_lock_.release();
    }
  }
 }


// bool UserThread::schedulable()
// {
//   bool running = (getState() == Running);
//   if(running)
//   {
//     // if the thread requests to sleep, then it is not scheduled
//     if (request_to_sleep_ && *(size_t*) request_to_sleep_ == 1)
//     {
//         return false;
//     }
    
//     if(wakeup_timestamp_ == 0)
//     {
//       return true;
//     }
//     else
//     {
//       unsigned int edx;
//       unsigned int eax;
//       asm
//       (
//         "rdtsc"
//         : "=a"(eax), "=d"(edx)
//       );
//       unsigned long current_time_stamp = ((unsigned long)edx<<32) + eax;
//       if(current_time_stamp >= wakeup_timestamp_)
//       {
//         wakeup_timestamp_ = 0;
//         return true;
//       }
//     }
//   }
//   return false;
// }