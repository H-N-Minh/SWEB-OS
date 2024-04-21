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
                       Loader* loader, UserProcess* process, void* func, void* arg, void* pcreate_helper)
            : Thread(working_dir, name, type, loader), process_(process), thread_gets_killed_lock_("thread_gets_killed_lock_"), 
              thread_gets_killed_(&thread_gets_killed_lock_, "thread_gets_killed_"), join_state_lock_("join_state_lock_"),
              cancel_state_type_lock_("cancel_state_type_lock_"), guarded_(0)
{
    tid_ = ArchThreads::atomic_add(UserProcess::tid_counter_, 1);
 
    page_for_stack_ = PageManager::instance()->allocPPN();
    vpn_stack_ = USER_BREAK / PAGE_SIZE - tid_ * MAX_STACK_AMOUNT - 1;
    loader_->arch_memory_.lock_.acquire();
    bool vpn_mapped = loader_->arch_memory_.mapPage(vpn_stack_, page_for_stack_, 1);
    loader_->arch_memory_.lock_.release();
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    user_stack_ptr_ = (size_t) (USER_BREAK - MAX_STACK_AMOUNT * PAGE_SIZE * tid_ - (META_SIZE + 1) * sizeof(pointer));
    debug(USERTHREAD, "Userthread ctor: Reserving space for meta data at beginning of stack. (2 for Goards and 4 for locking)\n");
    top_stack_ = user_stack_ptr_ + 6 * sizeof(pointer);       // 1. Guard
    mutex_flag_ = user_stack_ptr_ + 5 * sizeof(pointer);      // 2. Mutex flag
    //                                                          3. Mutex waiter list
    cond_flag_ = user_stack_ptr_ + 3 * sizeof(pointer);       // 4. Cond flag
    //                                                          5. Cond waiter list
    //                                                          6. Guard
    //                                                          7. user_stack_ptr
    debug(USERSPACE_LOCKS, "UserStackPointer %zd(=%zx) and position for waiting flag  %zd(=%zx).\n",
          user_stack_ptr_, user_stack_ptr_, mutex_flag_, mutex_flag_);


    if (!func) //for the first thread when we create a process
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                         (void*) user_stack_ptr_, getKernelStackStartPointer());

        debug(USERTHREAD, "Create First thread: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
               user_stack_ptr_, user_stack_ptr_, vpn_stack_, vpn_stack_);
    }
    else // create the thread for pthread create
    {
        ArchThreads::createUserRegisters(user_registers_, (void*) pcreate_helper, (void*) user_stack_ptr_,
                                         getKernelStackStartPointer());

        user_registers_->rdi = (size_t)func;
        user_registers_->rsi = (size_t)arg;
        user_registers_->rdx = top_stack_; // address of the top of stack, relevant for userspace locks
        debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",
                user_stack_ptr_, user_stack_ptr_, vpn_stack_, vpn_stack_);

        debug(GROW_STACK, "UserThread ctor for pthread_create: Child guard is set up immediately (in userspace)\n");
        guarded_ = 1;
    }



  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));


  switch_to_userspace_ = 1;
}


UserThread::UserThread(UserThread& other, UserProcess* new_process)
            : Thread(other, new_process->loader_), process_(new_process), vpn_stack_(other.vpn_stack_), user_stack_ptr_(other.user_stack_ptr_),
             page_for_stack_(other.page_for_stack_),
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
}


void UserThread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", getName(),
        this, getName(), this);

  assert(currentThread == this && "Only the thread itself can kill itself\n");

  // FOR PTHREAD JOIN
  process_->threads_lock_.acquire();
  send_kill_notification();
  process_->threads_lock_.release();

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  ArchInterrupts::enableInterrupts();
  Scheduler::instance()->yield();
  assert(false && "This should never happen, how are we still alive?");
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

  if(wants_to_be_canceled_ && switch_to_userspace_
    && (cancel_type_ == PTHREAD_CANCEL_EXIT || (cancel_type_ == PTHREAD_CANCEL_ASYNCHRONOUS && cancel_state_ == PTHREAD_CANCEL_ENABLE))) 
  {
    debug(SCHEDULER, "Scheduler::schedule: Thread %s wants to be canceled, and is allowed to be canceled\n", getName());

    *request_to_sleep_translated = 0;
    *thread_waiting_for_lock_ptr = 0;
    
    kernel_registers_->rip     = (size_t)Syscall::pthreadExit;
    kernel_registers_->rdi     = (size_t)-1;
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

int UserThread::joinThread(size_t thread_id, void**value_ptr)
{
  debug(USERTHREAD, "UserThread:joinThread: called, thread_id: %zu and %p\n", thread_id, value_ptr);

  if(!Syscall::check_parameter((size_t)value_ptr, true) || (currentThread->getTID() == thread_id))
  {
    debug(USERTHREAD, "UserThread:pthreadJoin: Thread tries to join itself or invalid value_ptr.\n");
    return -1;
  }


  process_->threads_lock_.acquire();

  //check if thread is running
  UserThread* thread_to_be_joined = process_->getUserThread(thread_id);
  if(!thread_to_be_joined)
  {
    debug(USERTHREAD, "UserThread:pthreadJoin: No running thread id %zu can be found.\n", thread_id);

   //check if thread has already terminated 
    int thread_in_retval_map = process_->removeRetvalFromMapAndSetReval(thread_id, value_ptr);
    process_->threads_lock_.release();
    return thread_in_retval_map;
  }
  thread_to_be_joined->join_state_lock_.acquire();
  if(thread_to_be_joined->join_state_ != PTHREAD_CREATE_JOINABLE && thread_to_be_joined->join_state_ != PCJ_TO_BE_JOINED)
  {
    thread_to_be_joined->join_state_lock_.release();
    process_->threads_lock_.release();
    return -1;
  }
  else 
  {
    thread_to_be_joined->join_state_ = PCJ_TO_BE_JOINED;
  }

  thread_to_be_joined->join_state_lock_.release();
  thread_gets_killed_lock_.acquire();
  thread_to_be_joined->join_threads_.push_back(this);
  process_->threads_lock_.release();
  
  //wait for thread get killed
  while(!thread_killed)
  {
    thread_gets_killed_.wait();
  }     
  thread_killed = false;               
  thread_gets_killed_lock_.release(); 

  process_->threads_lock_.acquire();
  int thread_in_retval_map = process_->removeRetvalFromMapAndSetReval(thread_id, value_ptr);
  process_->threads_lock_.release();

  return thread_in_retval_map;
}

void UserThread::exitThread(void* value_ptr)
{
  //remove thread from process' thread vector
  process_->threads_lock_.acquire();
  ustl::vector<UserThread*>::iterator exiting_thread_iterator = ustl::find(process_->threads_.begin(), process_->threads_.end(), this);
  process_->threads_.erase(exiting_thread_iterator);

  if(process_->threads_.size() == 0)  // last thread in process
  {
    debug(USERTHREAD, "UserThread::exitThread: last thread alive\n");
    last_thread_alive_ = true;
    process_->thread_retval_map_.clear();
  }

  join_state_lock_.acquire();
  if(join_state_ != PTHREAD_CREATE_DETACHED && !last_thread_alive_)  //Todos: cleanup one thread left
  {
    debug(USERTHREAD, "UserThread::exitThread: saving return value in thread_retval_map_ in case the thread is joinable\n");
    process_->thread_retval_map_[getTID()] = value_ptr;
  }
  join_state_lock_.release();


  if(process_->threads_.size() == 1)  // only one thread left
  {
    process_->one_thread_left_lock_.acquire();
    process_->one_thread_left_ = true;
    process_->one_thread_left_condition_.signal();
    process_->one_thread_left_lock_.release();

  }

  debug(SYSCALL, "pthreadExit: Thread %ld unmapping thread's virtual page, then kill itself\n",getTID());
  process_->unmapThreadStack(&loader_->arch_memory_, top_stack_);
  process_->threads_lock_.release();
  kill();

}

int UserThread::createThread(size_t* thread, void* start_routine, void* wrapper, void* arg, unsigned int* attr)
{ 
  if(!Syscall::check_parameter((size_t)thread) || !Syscall::check_parameter((size_t)attr, true) 
  || !Syscall::check_parameter((size_t)start_routine) || !Syscall::check_parameter((size_t)arg, true) 
  || !Syscall::check_parameter((size_t)wrapper))
  {
    return -1;
  }
  debug(USERPROCESS, "UserThread::createThread: func (%p), para (%zu) \n", start_routine, (size_t) arg);

  process_->threads_lock_.acquire();  
  UserThread* new_thread = new UserThread(process_->working_dir_, process_->filename_, Thread::USER_THREAD, process_->terminal_number_,
                                          process_->loader_, process_, start_routine, arg, wrapper);
  if(new_thread)
  {
    debug(USERPROCESS, "UserThread::createThread: Adding new thread to scheduler\n");
    process_->threads_.push_back(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    *thread = new_thread->getTID();
    process_->threads_lock_.release();  
    return 0;
  }
  else
  {
    debug(USERPROCESS, "UserThread::createThread: ERROR: Thread not created\n");
    process_->threads_lock_.release();  
    return -1;
  }
}

int UserThread::cancelThread(size_t thread_id)
{
  assert(process_->threads_lock_.heldBy() == this && "Threads lock needs to be held when canceling threads");
  UserThread* thread_to_be_canceled = process_->getUserThread(thread_id);
  if(!thread_to_be_canceled)
  {
    debug(USERTHREAD, "UserThread::cancelThread: thread_id %zu doesnt exist in Vector\n", thread_id);
    return -1;
  }
  debug(USERTHREAD, "UserThread::cancelThread: thread_id %zu setted to be canceled\n", thread_id);
  thread_to_be_canceled->cancel_state_type_lock_.acquire();
  thread_to_be_canceled->wants_to_be_canceled_ = true;
  thread_to_be_canceled->cancel_state_type_lock_.release();
  return 0;
}

int UserThread::detachThread(size_t thread_id)
{
  assert(process_->threads_lock_.heldBy() == this && "Threads lock needs to be held when detaching threads");
  debug(USERTHREAD, "UserThread::detachThread: called, thread_id: %zu\n", thread_id);
  UserThread* thread_to_be_detached = process_->getUserThread(thread_id);
  if(thread_to_be_detached)
  {
    thread_to_be_detached ->join_state_lock_.acquire();
    if(thread_to_be_detached->join_state_ == PTHREAD_CREATE_DETACHED || thread_to_be_detached->join_state_ == PCD_TO_BE_JOINED)
    {
      thread_to_be_detached->join_state_lock_.release();
      return -1;    // thread is already detached
    }

    else if(thread_to_be_detached->join_state_ == PTHREAD_CREATE_JOINABLE)
    {
      thread_to_be_detached->join_state_ = PTHREAD_CREATE_DETACHED;
    }

    else //(thread_to_be_detached->join_state_ == PCJ_TO_BE_JOINED)
    {
      thread_to_be_detached->join_state_ = PCD_TO_BE_JOINED;
    }
    thread_to_be_detached->join_state_lock_.release();
    return 0;
  }
  else
  {
    int thread_in_retval_map = process_->removeRetvalFromMapAndSetReval(thread_id, NULL);
    return thread_in_retval_map;

  }

  return 0;
}


