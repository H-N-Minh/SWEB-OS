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


// TODO: explain calculation related to execv()
UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, size_t tid, void* func, void* arg, void* pcreate_helper, bool execv)
            : Thread(working_dir, name, type, loader), process_(process) , cancel_state_type_lock_("cancel_state_type_lock_")
                // TODO: add these. NOTE: PUT BEFORE cancel_state_type_lock_: join_threads_lock_("join_threads_lock_"), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
                //thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_")
{
    tid_ = tid;
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
    virtual_page_ = USER_BREAK / PAGE_SIZE - tid - 1;
    bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page_, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    size_t user_stack_ptr = (size_t) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * (tid));

    if (!func) //for the first thread when we create a process
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                         (void*) user_stack_ptr, getKernelStackStartPointer());

        debug(USERTHREAD, "Create First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
               user_stack_ptr, user_stack_ptr, virtual_page_, virtual_page_);
    }
    else // create the thread for every pthread create
    {
        ArchThreads::createUserRegisters(user_registers_, (void*) pcreate_helper, (void*) user_stack_ptr,
                                         getKernelStackStartPointer());

        user_registers_->rdi = (size_t)func;
        user_registers_->rsi = (size_t)arg;
        debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
                user_stack_ptr, user_stack_ptr, virtual_page_, virtual_page_);
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



// TODO MINH: correct this to fit new constructor
UserThread::UserThread(UserThread& other, UserProcess* new_process, int32 tid, uint32 terminal_number, Loader* loader)
        : Thread(other, loader), process_(new_process), cancel_state_type_lock_("cancel_state_type_lock_")
{
    debug(USERTHREAD, "UserThread COPY-Constructor: start copying from thread (%zu) \n", other.getTID());
    tid_ = tid;
    virtual_page_ = other.virtual_page_;

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
        size_t new_thread_id = ArchThreads::atomic_add(process_->tid_counter_, 1);
        UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_, process_->loader_, process_, new_thread_id, 0, 0, 0, true);
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
//     if(!current_process.isThreadInVector(join_thread))
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
