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
#include "Syscall.h"
#include "ArchMemory.h"
#include "UserSpaceMemoryManager.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, void* func, void* attr, void* arg, void* pcreate_helper, bool execv)
            : Thread(working_dir, name, type, loader), process_(process), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
              thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"), join_state_lock_("join_state_lock_"),
              cancel_state_type_lock_("cancel_state_type_lock_"), guarded_(0)
{
  tid_ = ArchThreads::atomic_add(UserProcess::tid_counter_, 1);

  if(execv)
  {
    size_t virtual_address =  ArchMemory::getIdentAddressOfPPN(process_->execv_ppn_args_);
    debug(USERTHREAD, "Value of %s.\n", ((char*)virtual_address));

    size_t virtual_page = USER_BREAK / PAGE_SIZE - 1;
    loader_->arch_memory_.lock_.acquire();
    bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page , process_->execv_ppn_args_, 1);
    loader_->arch_memory_.lock_.release();
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen - in execv");
  }

    size_t page_for_stack = PageManager::instance()->allocPPN();
    vpn_stack_ = USER_BREAK / PAGE_SIZE - tid_ * MAX_STACK_AMOUNT - 1;

    loader_->arch_memory_.lock_.acquire();
    bool vpn_mapped = loader_->arch_memory_.mapPage(vpn_stack_, page_for_stack, 1);
    loader_->arch_memory_.lock_.release();
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

  void* user_stack_ptr1 = (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * (tid_-1));
  debug(TAI_THREAD, "-------------------user_stack_ptr1 (%p) \n", user_stack_ptr1);

    size_t user_stack_ptr = (size_t) (USER_BREAK - MAX_STACK_AMOUNT * PAGE_SIZE * tid_ - (META_SIZE + 1) * sizeof(pointer));

    debug(USERTHREAD, "Userthread ctor: Reserving space for meta data at beginning of stack. (2 for Goards and 4 for locking)\n");
    top_stack_ = user_stack_ptr + 6 * sizeof(pointer);       // 1. Guard
    mutex_flag_ = user_stack_ptr + 5 * sizeof(pointer);      // 2. Mutex flag
    //                                                          3. Mutex waiter list
    cond_flag_ = user_stack_ptr + 3 * sizeof(pointer);       // 4. Cond flag
    //                                                          5. Cond waiter list
    //                                                          6. Guard
    //                                                          7. user_stack_ptr
    debug(USERSPACE_LOCKS, "UserStackPointer %zd(=%zx) and position for waiting flag  %zd(=%zx).\n",
          user_stack_ptr, user_stack_ptr, mutex_flag_, mutex_flag_);
    if (!func) //for the first thread when we create a process
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                         (void*) user_stack_ptr, getKernelStackStartPointer());

      if(execv)
      {
        user_registers_->rdi = process_->exec_argc_;
        user_registers_->rsi = USER_BREAK - PAGE_SIZE + process_->exec_array_offset_;
      }

          debug(USERTHREAD, "Create First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
                 user_stack_ptr, user_stack_ptr, vpn_stack_, vpn_stack_);
    }
    else // create the thread for pthread create
    {
      if(attr)
      {
        pthread_attr_t* attr_k;
        attr_k = reinterpret_cast<pthread_attr_t*>(attr);

        debug(TAI_THREAD, "-------------------UserThread::UserThread: attr (%p) \n", attr_k);
        debug(TAI_THREAD, "-------------------UserThread::UserThread: attrk (%p) \n", attr_k);

        size_t stack_size = attr_k->stack_size;
        debug(TAI_THREAD, "-------------------UserThread::UserThread: stack_size (%zu) \n", stack_size);
      }

      ArchThreads::createUserRegisters(user_registers_, (void*) pcreate_helper, (void*) user_stack_ptr,
                                       getKernelStackStartPointer());

      user_registers_->rdi = (size_t)func;
      user_registers_->rsi = (size_t)arg;
      user_registers_->rdx = top_stack_; // address of the top of stack, relevant for userspace locks
      debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
              user_stack_ptr, user_stack_ptr, vpn_stack_, vpn_stack_);

      debug(GROW_STACK, "UserThread ctor for pthread_create: Child guard is set up immediately (in userspace)\n");
      guarded_ = 1;
    }



  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));


  switch_to_userspace_ = 1;
}


UserThread::UserThread(UserThread& other, UserProcess* new_process)
            : Thread(other, new_process->loader_), process_(new_process), vpn_stack_(other.vpn_stack_),
            thread_gets_killed_lock_("thread_gets_killed_lock_"),  thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"),
            join_state_lock_("join_state_lock_"), join_state_(other.join_state_), cancel_state_type_lock_("cancel_state_type_lock_"),
            cancel_state_(other.cancel_state_), cancel_type_(other.cancel_type_),
            cond_flag_(other.cond_flag_), mutex_flag_(other.mutex_flag_), guarded_(other.guarded_), top_stack_(other.top_stack_)
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

  assert(join_threads_.size() == 0 && "There are still waiting threads to get joined, but this is last thread");

  if(last_thread_alive_)
  {
    assert(process_->threads_.size() == 0 && "Not all threads removed from threads_");
    assert(process_->thread_retval_map_.size() == 0 && "There are still values in retval map");
    debug(USERTHREAD, "Userprocess gets destroyed by thread with id %ld.\n", getTID());
    delete process_;
    process_ = 0;
  }

  if(unlikely(last_thread_before_exec_))
  {
    assert(process_->threads_.size() == 0 && "Not all threads removed from threads_");
    assert(process_->thread_retval_map_.size() == 0 && "There are still values in retval map");

    debug(USERTHREAD, "Last thread %ld before exec get destroyed.\n", getTID());
    assert(Scheduler::instance()->isCurrentlyCleaningUp());
    delete loader_;
    process_->loader_ = process_->execv_loader_;
    process_->execv_loader_ = 0;

    VfsSyscall::close(process_->fd_);

    process_->fd_ = process_->execv_fd_;

    UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_, process_->loader_, process_,0,0, 0, 0, true);
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
  currentUserThread.join_threads_.clear();
}

bool UserThread::schedulable()
{
  bool running = (getState() == Running);

  int waiting_for_lock = 0;

  // if the thread requests to sleep, then it is not scheduled
  size_t *request_to_sleep_translated = (size_t*)loader_->arch_memory_.checkAddressValid((uint64)cond_flag_);
  if(request_to_sleep_translated && *request_to_sleep_translated == 1)
  {
    waiting_for_lock = 1;
  }

  size_t *thread_waiting_for_lock_ptr = (size_t*)loader_->arch_memory_.checkAddressValid((uint64)mutex_flag_);

  //debug(USERSPACE_LOCKS, "Kernel: userspace %zd(=%zx)\n", (uint64)thread_waiting_for_lock_ptr, (uint64)thread_waiting_for_lock_ptr);
  if(thread_waiting_for_lock_ptr && *thread_waiting_for_lock_ptr == 1)
  {
    waiting_for_lock = 1;
  }

  if(wants_to_be_canceled_
      && (switch_to_userspace_ || waiting_for_lock)
      && (cancel_type_ == PTHREAD_CANCEL_EXIT || (cancel_type_ == PTHREAD_CANCEL_ASYNCHRONOUS && cancel_state_ == PTHREAD_CANCEL_ENABLE)))
  {
    debug(SCHEDULER, "Scheduler::schedule: Thread %s wants to be canceled, and is allowed to be canceled\n", getName());
    if (!switch_to_userspace_ && waiting_for_lock)  // TODO: maybe find a cleaner way to do this
    {
      *request_to_sleep_translated = 0;
      *thread_waiting_for_lock_ptr = 0;
      return true;
    }

    kernel_registers_->rip     = (size_t)Syscall::pthreadExit;
    kernel_registers_->rdi     = (size_t)-1;
    currentThreadRegisters = currentThread->kernel_registers_;   //TODOs ???
    switch_to_userspace_ = 0;
    return true;
  }

  if (waiting_for_lock)
  {
    return false;
  }

  if(!running)
  {
    return false;
  }

  if(wakeup_timestamp_ == 0)
  {
    return true;
  }
  else
  {
    unsigned long current_time_stamp =  Syscall::get_current_timestamp_64_bit();
    if(current_time_stamp >= wakeup_timestamp_)
    {
      wakeup_timestamp_ = 0;
      return true;
    }
    else
    {
      return false;
    }
  }
}

