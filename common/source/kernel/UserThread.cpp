#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "ProcessRegistry.h"
#include "debug.h"
#include "PageManager.h"
#include "VfsSyscall.h"
#include "ustring.h"
#include "UserProcess.h"
#include "ArchInterrupts.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, 
            void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter, bool execv):
            Thread(working_dir, name, type, loader), process_(process), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
            thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"), 
            has_recieved_pthread_exit_notification_lock_("has_recieved_pthread_exit_notification_lock_"),
            has_recieved_pthread_exit_notification_(&has_recieved_pthread_exit_notification_lock_, "has_recieved_pthread_exit_notification_")
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
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    setTID(thread_counter);

    switch_to_userspace_ = 1; 

}


UserThread::UserThread(UserThread const &src, UserProcess* process, size_t thread_counter) : 
            Thread(src, process), process_(process), last_thread_alive_(false), last_thread_before_exec_(false), wants_to_be_canceled_(false),
            exit_send_cancelation_(false), join_thread_(NULL), thread_killed(false), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
            thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"), canceled_thread_wants_to_be_killed_(false), 
            cancel_threads_(NULL), cancel_state_(src.cancel_state_), cancel_type_(src.cancel_type_), 
            recieved_pthread_exit_notification_(false), has_recieved_pthread_exit_notification_lock_("has_recieved_pthread_exit_notification_lock_"),
            has_recieved_pthread_exit_notification_(&has_recieved_pthread_exit_notification_lock_, "has_recieved_pthread_exit_notification_")
{
    debug(FORK, "Copy constructor UserThread\n");



    //size_t page_for_stack = PageManager::instance()->allocPPN();
    //virtual_page_ = USER_BREAK / PAGE_SIZE - thread_counter - 1;
    //size_t user_stack_start = USER_BREAK - sizeof(pointer) - PAGE_SIZE * (thread_counter);
    //bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page_ , page_for_stack, 1);
    //assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen - in thread");
    

    //ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*)user_stack_start, getKernelStackStartPointer());
    //debug(USERTHREAD, "First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",user_stack_start, user_stack_start, virtual_page_, virtual_page_);
    

    //ArchThreads::setAddressSpace(this, loader_->arch_memory_); 
    //if (main_console->getTerminal(terminal_number))
    //    setTerminal(main_console->getTerminal(terminal_number));

    setTID(thread_counter);
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

        // delete working_dir_;     //not sure if i have to close thise
        // working_dir_ = 0;

        assert(process_->threads_.size() == 1);
        process_->threads_.clear();
        process_->thread_counter_++;          //TODO: not sure if i have to lock since it should be singlethreaded (same for next lines)
        UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_, process_->loader_, process_, 0, 0, 0, process_->thread_counter_, true);
        process_->threads_.push_back(new_thread);
        Scheduler::instance()->addNewThread(new_thread);
    }

}

void UserThread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

  if(join_thread_)
  {
    UserThread& join_thread = *join_thread_;
    join_thread_ = NULL;  
    join_thread.thread_gets_killed_lock_.acquire();
    join_thread.thread_killed = true;                                         //TODO do need to lock
    join_thread.thread_gets_killed_.signal();
    join_thread.thread_gets_killed_lock_.release();
  }

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "This should never happen, how are we still alive?");
  }
}

void UserThread::Run(){
    assert(0);
}
