#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"
#include "uvector.h"
#include "Mutex.h"
#include "Loader.h"
#include "umap.h"
#include "ArchMemory.h"
#include "PageManager.h"

#include "ArchThreads.h"

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;
  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline)) // no debug print because these might occur very often
  {
    debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
          syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  }
  UserThread& currentUserThread = *((UserThread*)currentThread);
  currentUserThread.cancel_state_type_lock_.acquire();
  if(currentUserThread.wants_to_be_canceled_)
  {
    if(currentUserThread.cancel_type_ == PTHREAD_CANCEL_EXIT)
    {
      currentUserThread.cancel_state_type_lock_.release();
      pthreadExit((void*)-2222222222);
    }
    else if (currentUserThread.cancel_state_ == PTHREAD_CANCEL_ENABLE)
    {
      currentUserThread.cancel_state_type_lock_.release();
      pthreadExit((void*)-1111111111);
    }
  }
  currentUserThread.cancel_state_type_lock_.release();

  switch (syscall_number)
  {
    case sc_sched_yield:
      Scheduler::instance()->yield();
      break;
    case sc_createprocess:
      return_value = createprocess(arg1, arg2);
      break;
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value = write(arg1, arg2, arg3);
      break;
    case sc_read:
      return_value = read(arg1, arg2, arg3);
      break;
    case sc_open:
      return_value = open(arg1, arg2);
      break;
    case sc_close:
      return_value = close(arg1);
      break;
    case sc_outline:
      outline(arg1, arg2);
      break;
    case sc_trace:
      trace();
      break;
    case sc_pseudols:
      pseudols((const char*) arg1, (char*) arg2, arg3);
      break;
    case sc_threadcount:
      return_value = get_thread_count();
      break;
    case sc_pthread_create:
      return_value = pthread_create((size_t*)arg1, (unsigned int*) arg2, (void*) arg3, (void*) arg4, (void*)arg5);
      break;
    case sc_pthread_exit:
      pthreadExit((void*)arg1);
      break;  
    case sc_pthread_join:
      return_value = pthread_join((size_t)arg1, (void **)arg2);
      //return_value = joinThread(arg1, (void**) arg2);  //from Minh
      break;
    case sc_pthread_cancel:
      return_value = pthread_cancel((size_t)arg1);
      break; 
    case sc_sleep:
      return_value = sleep((unsigned int)arg1);
      break;
    case sc_execv:
      return_value = execv((const char *)arg1, (char *const*)arg2);
      break;
    case sc_pthread_setcancelstate:
      return_value = pthread_setcancelstate((int)arg1, (int *)arg2);
      break;
    case sc_pthread_setcanceltype:
      return_value = pthread_setcanceltype((int)arg1, (int *)arg2);
      break;
    case sc_pthread_testcancel:
      break;
    case sc_fork:
      return_value = forkProcess();
      break; // you will need many debug hours if you forget the break
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }

  return return_value;
}

// TODO: handle return value when fork fails, handle how process exits correctly after fork
uint32 Syscall::forkProcess()
{
  // Scheduler::instance()->printThreadList();
  debug(FORK, "Syscall::forkProcess: start focking \n");
  UserProcess* parent = ((UserThread*) currentThread)->process_;
  UserProcess* child = new UserProcess(*parent);

  // Scheduler::instance()->printThreadList();

  if (!child)
  {
    debug(SYSCALL, "Syscall::forkProcess: fock failed \n");
    return -1;
  }
  else
  {
    debug(SYSCALL, "Syscall::forkProcess: fock done with return (%d) \n", (uint32) currentThread->user_registers_->rax);
    return (uint32) currentThread->user_registers_->rax;
  }
}


void Syscall::pthreadExit(void* value_ptr, bool from_exec)
{
  debug(SYSCALL, "Syscall::pthreadExit: called, value_ptr: %p and from exec %d\n", value_ptr, from_exec);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  // remove thread from process' thread vector
  current_process.threads_lock_.acquire();
  ustl::vector<UserThread*>::iterator exiting_thread = ustl::find(current_process.threads_.begin(), current_process.threads_.end(), currentThread);
  current_process.threads_.erase(exiting_thread);

  if(current_process.threads_.size() == 0)  // last thread in process
  {
    if(!from_exec)
    {
      debug(SYSCALL, "Syscall::pthreadExit: last thread alive\n");
      currentUserThread.last_thread_alive_ = true;
    }
    else
    {
       debug(SYSCALL, "Syscall::pthreadExit: last thread before exec\n");
      currentUserThread.last_thread_before_exec_ = true;
    }
  }
  else  // not last thread in process, saving return values in thread_retval_map_
  {
    debug(SYSCALL, "Syscall::pthreadExit: saving return value in thread_retval_map_\n");
    current_process.thread_retval_map_lock_.acquire();
    current_process.thread_retval_map_[currentUserThread.getTID()] = value_ptr;
    current_process.thread_retval_map_lock_.release();
  }

  // TODO: Lock arch_memory_, also be careful with locking order to prevent deadlock
  debug(SYSCALL, "pthreadExit: Thread %ld unmapping thread's virtual page, then kill itself\n",currentUserThread.getTID());
  currentUserThread.loader_->arch_memory_.unmapPage(currentUserThread.virtual_page_);
  current_process.threads_lock_.release();
  currentUserThread.kill();
  assert(false && "This should never happen");
}


int Syscall::pthread_join(size_t thread_id, void**value_ptr)
{
  debug(SYSCALL, "Syscall::PTHREAD_JOIN: called, thread_id: %zu and %p\n", thread_id, value_ptr);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  if(!check_parameter((size_t)value_ptr, true))
  {
    return -1;
  }

  //checks if I not accidentally join myself
  if(currentThread->getTID() == thread_id)                         
  {
    return -1;
  }

  //check if thread has already terminated
  current_process.threads_lock_.acquire();
  current_process.thread_retval_map_lock_.acquire();  
  ustl::map<size_t, void*>::iterator iterator = current_process.thread_retval_map_.find(thread_id);                                                   
  void* return_value;
  if(iterator != current_process.thread_retval_map_.end())
  {
    return_value = current_process.thread_retval_map_[thread_id];
    
    current_process.thread_retval_map_.erase(iterator);
    if(value_ptr != NULL)
    {
      *value_ptr = return_value;
    }
    current_process.thread_retval_map_lock_.release();
    current_process.threads_lock_.release(); 
    return 0;
  }
  current_process.thread_retval_map_lock_.release();

  //find thread in threadlist
  UserThread* thread_to_be_joined;
  for (auto& thread : current_process.threads_)
  {
    if(thread_id == thread->getTID())
    {
      thread_to_be_joined = thread;
      break;
    } 
  }
  if(!thread_to_be_joined)
  {
    current_process.threads_lock_.release();
    return -1;
  }

  currentUserThread.thread_gets_killed_lock_.acquire();
  thread_to_be_joined->join_threads_lock_.acquire();
  thread_to_be_joined->join_threads_.push_back(&currentUserThread);
  thread_to_be_joined->join_threads_lock_.release();


  current_process.threads_lock_.release();
  
  while(!currentUserThread.thread_killed)
  {
    currentUserThread.thread_gets_killed_.wait();
  }     
  currentUserThread.thread_killed = false;               
  currentUserThread.thread_gets_killed_lock_.release(); 


  current_process.thread_retval_map_lock_.acquire();  
  iterator = current_process.thread_retval_map_.find(thread_id);            
  if(iterator != current_process.thread_retval_map_.end())
  {
    return_value = current_process.thread_retval_map_[thread_id];
    if(return_value == (void*)-2222222222)  //maybe is not the best joince
    {
      current_process.thread_retval_map_lock_.release();  
      pthreadExit((void*)currentUserThread.getTID());
    }
    current_process.thread_retval_map_.erase(iterator);
    if(value_ptr != NULL)
    {
      *value_ptr = return_value;
    }
  }
  current_process.thread_retval_map_lock_.release();  
  return 0;
}



int Syscall::pthread_create(size_t* thread, unsigned int* attr, void* start_routine, void* arg, void* wrapper_address)         
{
  debug(SYSCALL, "Syscall::Pthread_CREATE Pthread_created called\n");
  if(!(check_parameter((size_t)thread) && check_parameter((size_t)attr, true) && check_parameter((size_t)start_routine) 
      && check_parameter((size_t)arg, true) && check_parameter((size_t)wrapper_address)))
  {
    return -1;
  }
  int rv = ((UserThread*) currentThread)->process_->createThread(thread, start_routine, wrapper_address, arg);
  debug(SYSCALL, "Syscall::Pthread_CREATE: finished with return (%d) for thread (%zu)\n", rv, *thread);
  return rv;
}



void Syscall::exit(size_t exit_code, bool from_exec)
{
  debug(SYSCALL, "Syscall::EXIT: Thread (%zu) called exit_code: %zd and from exec %d\n", currentThread->getTID(), exit_code, from_exec);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  current_process.threads_lock_.acquire();
  for (auto& thread : current_process.threads_)
  {
    if(thread != &currentUserThread)
    {
      thread->cancel_state_type_lock_.acquire();
      thread->cancel_type_ = PTHREAD_CANCEL_EXIT;  
      thread->cancel_state_type_lock_.release();
      debug(SYSCALL, "EXIT: Thread %zu gets canceled. \n",thread->getTID());
      pthread_cancel(thread->getTID(), true);
      debug(SYSCALL, "EXIT: Thread %zu was canceled sucessfully. \n",thread->getTID());
    } 
  }
  current_process.threads_lock_.release();

  debug(SYSCALL, "EXIT: Last Thread %zu calls pthread exit. \n",currentThread->getTID());
  pthreadExit((void*)exit_code, from_exec);
  assert(false && "This should never happen");
}


int Syscall::pthread_cancel(size_t thread_id, bool is_tVector_locked_in_Exit)
{
  debug(SYSCALL, "Syscall::PTHREAD_CANCEL: called with thread_id %ld and called from Exit?: (%d).\n",thread_id, is_tVector_locked_in_Exit);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  if(!is_tVector_locked_in_Exit)  {current_process.threads_lock_.acquire();}

  UserThread* thread_to_be_canceled = current_process.getUserThread(thread_id);
  if(!thread_to_be_canceled)
  {
    debug(SYSCALL, "Syscall::PTHREAD_CANCEL: thread_id %zu doesnt exist in Vector\n", thread_id);
    // if its not locked in exit then its locked in here and we need to release it
    if(!is_tVector_locked_in_Exit){current_process.threads_lock_.release();}
    return -1;
  }
  debug(SYSCALL, "Syscall::PTHREAD_CANCEL: thread_id %zu setted to be canceled\n", thread_id);
  thread_to_be_canceled->wants_to_be_canceled_ = true;
  if(!is_tVector_locked_in_Exit){current_process.threads_lock_.release();}
  return 0;
}


size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    return -1U;
  }

  size_t num_written = 0;

  if (fd == fd_stdout) //stdout
  {
    //debug(SYSCALL, "Syscall::write: %.*s\n", (int)size, (char*) buffer);
    kprintf("%.*s", (int)size, (char*) buffer);
    num_written = size;
  }
  else
  {
    num_written = VfsSyscall::write(fd, (char*) buffer, size);
  }
  return num_written;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
  {
    return -1U;
  }

  size_t num_read = 0;

  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", (int)num_read, (char*) buffer);
  }
  else
  {
    num_read = VfsSyscall::read(fd, (char*) buffer, count);
  }
  return num_read;
}

size_t Syscall::close(size_t fd)
{
  return VfsSyscall::close(fd);
}

size_t Syscall::open(size_t path, size_t flags)
{
  if (path >= USER_BREAK)
  {
    return -1U;
  }
  return VfsSyscall::open((char*) path, flags);
}

void Syscall::outline(size_t port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= USER_BREAK)
  {
    return;
  }
  if (port == 0xe9) // debug port
  {
    writeLine2Bochs((const char*) text);
  }
}

size_t Syscall::createprocess(size_t path, size_t sleep)
{
  // THIS METHOD IS FOR TESTING PURPOSES ONLY AND NOT MULTITHREADING SAFE!
  // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

  // parameter check begin
  if (path >= USER_BREAK)
  {
    return -1U;
  }

  debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%zd\n", (char*) path, sleep);
  ssize_t fd = VfsSyscall::open((const char*) path, O_RDONLY);
  if (fd == -1)
  {
    return -1U;
  }
  VfsSyscall::close(fd);
  // parameter check end

  size_t process_count = ProcessRegistry::instance()->processCount();
  ProcessRegistry::instance()->createProcess((const char*) path);
  if (sleep)
  {
    while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return 0;
}

void Syscall::trace()
{
  currentThread->printBacktrace();
}

uint32 Syscall::get_thread_count() {
    return Scheduler::instance()->getThreadCount();
}



void Syscall::pseudols(const char *pathname, char *buffer, size_t size)
{
  if(buffer && ((size_t)buffer >= USER_BREAK || (size_t)buffer + size > USER_BREAK))
    return;
  if((size_t)pathname >= USER_BREAK)
    return;
  VfsSyscall::readdir(pathname, buffer, size);
}


unsigned int Syscall::sleep(unsigned int seconds)
{
  return seconds;
}


bool Syscall::check_parameter(size_t ptr, bool allowed_to_be_null)
{
    if(!allowed_to_be_null && ptr == 0)
    {
      return false;
    }
    debug(SYSCALL, "Ptr %p USER_BREAK %p.\n",(void*)ptr, (void*)USER_BREAK);
    if(ptr >= USER_BREAK)
    {
      return false;
    }
    return true;
}

int Syscall::execv(const char *path, char *const argv[])
{
  if(!check_parameter((size_t)argv, false))
  {
    return -1;
  }
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  current_process.execv_fd_ = VfsSyscall::open(path, O_RDONLY);

  if (current_process.execv_fd_ >= 0)
  {
    current_process.execv_loader_ = new Loader(current_process.execv_fd_);
  }

  if (!current_process.execv_loader_)
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", path);
    VfsSyscall::close(current_process.execv_fd_);
    return -1;
    
  }
  if (!current_process.execv_loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", path);
    delete current_process.execv_loader_;
    current_process.execv_loader_ = 0;
    VfsSyscall::close(current_process.execv_fd_);
    return -1;
  }

  //map arguments to identity mapping
  current_process.execv_ppn_args_ = PageManager::instance()->allocPPN();
  size_t virtual_address =  ArchMemory::getIdentAddressOfPPN(current_process.execv_ppn_args_);

  size_t array_offset = 3500; //choose a good value

  size_t index = 0;
  size_t offset = 0;
  while(1)
  {
    if(argv[index] == NULL)
    {
      break;
    }
    if(!check_parameter((size_t)argv[index], true) || (offset +  strlen(argv[index]) + 1) >= array_offset || index >= 300)
    {
      delete current_process.execv_loader_;
      current_process.execv_loader_ = 0;
      VfsSyscall::close(current_process.execv_fd_);
      PageManager::instance()->freePPN(current_process.execv_ppn_args_);
      return -1;
    }

    memcpy((char*)virtual_address + offset, argv[index], strlen(argv[index])+1);
    memcpy((void*)(virtual_address + array_offset + index * sizeof(pointer)), &offset, sizeof(pointer));

    offset += strlen(argv[index]) + 1;

    index++;
  }
  current_process.exec_argc_ = index;

  exit(0, true);

  ((UserThread*)currentThread)->last_thread_before_exec_ = true;
  ((UserThread*)currentThread)->kill();

  
  assert(0 && "Sucessful exec should not return");
}

int Syscall::pthread_setcancelstate(int state, int *oldstate)
{
  if(state != CancelState::PTHREAD_CANCEL_DISABLE && state != CancelState::PTHREAD_CANCEL_DISABLE)
  {
    debug(SYSCALL, "Syscall::pthread_setcancelstate: given state is not recognizable\n");
    return -1;
  }
  debug(SYSCALL, "Syscall::pthread_setcancelstate: thread (%zu) is setted cancel state to (%d)\n", currentThread->getTID(), state);
  ((UserThread*) currentThread)->cancel_state_type_lock_.acquire();
  *oldstate = (int) ((UserThread*) currentThread)->getCancelState(); //the state the its currently have

  ((UserThread*) currentThread)->setCancelState((CancelState)state);

  debug(SYSCALL, "current state %s, previous state %s\n",
        state == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED",
        *oldstate == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED");
  ((UserThread*) currentThread)->cancel_state_type_lock_.release();
  return 0;
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
    if(type != 2 && type != 3) //not ASYNCHRONOUS or DEFERRED in userspace
    {
        debug(TAI_THREAD, "------------------Call cancel type fail\n");
        return -1;
    }
    ((UserThread*) currentThread)->cancel_state_type_lock_.acquire();
    CancelType previous_type = ((UserThread*) currentThread)->getCancelType(); //the type the its currently have
    *oldtype = (int)previous_type;

    ((UserThread*) currentThread)->setCancelType((CancelType)type);

    debug(TAI_THREAD, "------------------current type %s, previous type %s\n",
          type == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS",
          *oldtype == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS");
    ((UserThread*) currentThread)->cancel_state_type_lock_.release();
    return 0; //success
}