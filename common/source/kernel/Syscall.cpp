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
    if(currentUserThread.cancel_type_ == PTHREAD_CANCEL_EXIT || currentUserThread.cancel_state_ == PTHREAD_CANCEL_ENABLE)
    {
      currentUserThread.cancel_state_type_lock_.release();
      pthreadExit((void*)-1);
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
      return_value = pthreadJoin((size_t)arg1, (void **)arg2);
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
      break;
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }

  return return_value;
}

// TODO: handle return value when fork fails, handle how process exits correctly after fork
uint32 Syscall::forkProcess()
{
  debug(FORK, "Syscall::forkProcess: start focking \n");
  UserProcess* parent = ((UserThread*) currentThread)->process_;
  UserProcess* child = new UserProcess(*parent);

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


void Syscall::pthreadExit(void* value_ptr)
{
  debug(SYSCALL, "Syscall::pthreadExit: called, value_ptr: %p.\n", value_ptr);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  // remove thread from process' thread vector
  current_process.threads_lock_.acquire();
  ustl::vector<UserThread*>::iterator exiting_thread = ustl::find(current_process.threads_.begin(), current_process.threads_.end(), currentThread);
  current_process.threads_.erase(exiting_thread);

  if(current_process.threads_.size() == 0)  // last thread in process
  {
      debug(SYSCALL, "Syscall::pthreadExit: last thread alive\n");
      currentUserThread.last_thread_alive_ = true;
  }
  else  // not last thread in process, saving return values in thread_retval_map_
  {
    debug(SYSCALL, "Syscall::pthreadExit: saving return value in thread_retval_map_\n");
    current_process.thread_retval_map_[currentUserThread.getTID()] = value_ptr;
  }

  // TODO: Lock arch_memory_, also be careful with locking order to prevent deadlock
  debug(SYSCALL, "pthreadExit: Thread %ld unmapping thread's virtual page, then kill itself\n",currentUserThread.getTID());
  currentUserThread.loader_->arch_memory_.unmapPage(currentUserThread.vpn_stack_);
  current_process.threads_lock_.release();
  currentUserThread.kill();
  assert(false && "This should never happen");
}


int Syscall::pthreadJoin(size_t thread_id, void**value_ptr)
{
  debug(SYSCALL, "Syscall:pthreadJoin: called, thread_id: %zu and %p\n", thread_id, value_ptr);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  if((!check_parameter((size_t)value_ptr, true)) || (currentThread->getTID() == thread_id))
  {
    return -1;
  }
  debug(SYSCALL, "Syscall:pthreadJoin: finished, thread_id: %zu and %p\n", thread_id, value_ptr);
  return current_process.joinThread(thread_id, value_ptr);
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

  if(!from_exec)
  {
    debug(SYSCALL, "EXIT: Last Thread %zu calls pthread exit. \n",currentThread->getTID());
    pthreadExit((void*)exit_code);
    assert(false && "This should never happen");
  }

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
  uint64_t femtoseconds = (uint64_t)seconds * 1000000000000000;
  debug(SYSCALL, "Want to sleep for %d seconds, which are %ld femtoseconds. \n", seconds, femtoseconds);
  unsigned int edx;
  unsigned int eax;
  asm
  (
    "rdtsc"
     : "=a"(eax), "=d"(edx)
  );

  uint64_t current_time_stamp = ((uint64_t)edx<<32) + eax;
  //debug(SYSCALL, "TSC is %ld.\n", current_time_stamp);

  uint64_t timestamp_fs = Scheduler::instance()->timestamp_fs_;
  //debug(SYSCALL, "Timestamp_ns is %ld.\n", timestamp_fs);

  currentThread->wakeup_timestamp_ = current_time_stamp + ( femtoseconds / timestamp_fs);
  //debug(SYSCALL, "Wakeup timestamp is %ld.\n", currentThread->wakeup_timestamp_);

  Scheduler::instance()->yield();
  return 0;
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
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  if(!check_parameter((size_t)argv, false))
  {
    return -1;
  }
  return current_process.execvProcess(path, argv);
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
  *oldstate = (int) ((UserThread*) currentThread)->cancel_state_;

  ((UserThread*) currentThread)->cancel_state_ = (CancelState)state;

  debug(SYSCALL, "current state %s, previous state %s\n",
        state == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED",
        *oldstate == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED");
  ((UserThread*) currentThread)->cancel_state_type_lock_.release();
  return 0;
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
    if(type != CancelType::PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED)
    {
        debug(SYSCALL, "Syscall::pthread_setcanceltype: given type is not recognizable\n");
        return -1;
    }
    ((UserThread*) currentThread)->cancel_state_type_lock_.acquire();
    CancelType previous_type = ((UserThread*) currentThread)->cancel_type_;
    *oldtype = (int)previous_type;
    ((UserThread*) currentThread)->cancel_type_ = (CancelType)type;

    debug(SYSCALL, "current type %s, previous type %s\n",
          type == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS",
          *oldtype == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS");
    ((UserThread*) currentThread)->cancel_state_type_lock_.release();
    return 0;
}