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

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;
  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline)) // no debug print because these might occur very often
  {
    debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
          syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  }
  if(((UserThread*)currentThread)->wants_to_be_canceled_)
  {
    if(((UserThread*)currentThread)->cancel_type_ == PTHREAD_CANCEL_DEFERRED)
    {
      pthread_exit((void*)7);   //TODO: should not be 7
    }
    else
    {
      Scheduler::instance()->yield();
    }
    
  }

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
      return_value = pthread_create((size_t*)arg1, (unsigned int*)arg2, (void *(*)(void*))arg3, (void*)arg4, (void *(*)())arg5);
      break;
    case sc_pthread_exit:
      pthread_exit((void*)arg1);
      break;  
    case sc_pthread_join:
      return_value = pthread_join((size_t)arg1, (void **)arg2);
      break;
    case sc_pthread_cancel:
      return_value = pthread_cancel((size_t)arg1);
      break; // you will need many debug hours if you forget the break
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

    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  if(((UserThread*)currentThread)->wants_to_be_canceled_)
  {
    pthread_exit((void*)8);            //TODO: should not be 8
  }
  return return_value;
}



void Syscall::exit(size_t exit_code)   
{
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %zd\n", exit_code);
  UserProcess& current_process = *currentThread->process_;
  while(1)
  {
    current_process.threads_lock_.acquire();
    debug(SYSCALL, "Syscall::EXIT LOCK\n");
    if(current_process.threads_.size() > 1)
    {
      size_t cancel_id = current_process.threads_[0]->getTID();
      if(cancel_id == currentThread->getTID())
      {
        cancel_id = current_process.threads_[1]->getTID();
      }
      debug(SYSCALL, "EXIT: Thread %ld gets canceled. \n",cancel_id);
      current_process.threads_lock_.release();
      ((UserThread*)currentThread)->cancel_type_ = PTHREAD_CANCEL_ASYNCHRONOUS; //TODO should be done assyncronous
      pthread_cancel(cancel_id);
      debug(SYSCALL, "EXIT: Thread %ld was canceled sucessfully. \n",cancel_id);
    }
    else
    {
      current_process.threads_lock_.release();
      break;
    }  
  }

  ((UserThread*)currentThread)->last_thread_alive_ = true;
  debug(SYSCALL, "Syscall::EXIT: Last Thread %ld gets killed. \n",currentThread->getTID());
  currentThread->kill();

  assert(false && "This should never happen");

}


int Syscall::pthread_join(size_t thread_id, void**value_ptr) //probably broken
{
  debug(SYSCALL, "Syscall::PTHREAD_JOIN: called, thread_id: %ld\n", thread_id);
  UserProcess& current_process = *currentThread->process_;
  if(!check_parameter((size_t)value_ptr, true))
  {
    return -1;
  }

  if(currentThread->getTID() == thread_id)                   //checks if I not accidentally join myself
  {
    return -1;
  }

  current_process.threads_lock_.acquire();
  current_process.value_ptr_by_id_lock_.acquire();  
  ustl::map<size_t, void*>::iterator iterator = current_process.value_ptr_by_id_.find(thread_id);                                                   
  void* return_value;
  if(iterator != current_process.value_ptr_by_id_.end())
  {
    return_value = current_process.value_ptr_by_id_[thread_id];
    current_process.value_ptr_by_id_.erase(iterator);
  }
  current_process.value_ptr_by_id_lock_.release(); 
  if(return_value)                         //thread has already terminated
  {
    if(value_ptr != NULL)
    {
      *value_ptr = return_value;
    }
    current_process.threads_lock_.release(); 
    return 0;
  }
  
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
    return -1;
  }

  current_process.threads_lock_.release();   //possible later release
  
  

  currentThread->thread_gets_killed_lock_.acquire();
  thread_to_be_joined->join_thread_ = currentThread; //??
  while(!currentThread->thread_killed)
  {
    currentThread->thread_gets_killed_.wait();
  }                    
  currentThread->thread_gets_killed_lock_.release(); 

  current_process.value_ptr_by_id_lock_.acquire();  
  iterator = current_process.value_ptr_by_id_.find(thread_id);            
  if(iterator != current_process.value_ptr_by_id_.end())
  {
    return_value = current_process.value_ptr_by_id_[thread_id];
    current_process.value_ptr_by_id_.erase(iterator);
  }
  current_process.value_ptr_by_id_lock_.release();  
  if(value_ptr != NULL)
  {
    *value_ptr = return_value;
  }
  return 0;
}

void Syscall::pthread_exit(void* value_ptr){
  debug(SYSCALL, "Syscall::PTHREAD_EXIT: called, value_ptr: %p\n", value_ptr);
  UserProcess& current_process = *currentThread->process_;
  current_process.threads_lock_.acquire();
  debug(SYSCALL, "Syscall::PTHREAD_EXIT_LOCK\n");

  ustl::vector<UserThread*>::iterator iterator = ustl::find(current_process.threads_.begin(), current_process.threads_.end(), currentThread);
  current_process.threads_.erase(iterator);

  if(((UserThread*)currentThread)->wants_to_be_canceled_)
  {
    send_cancelation_notification();
  }
  if(current_process.threads_.size() == 0)
  {
    ((UserThread*)currentThread)->last_thread_alive_ = true;
  }
  else
  {
    current_process.value_ptr_by_id_lock_.acquire();
    current_process.value_ptr_by_id_[currentThread->getTID()] = value_ptr;
    current_process.value_ptr_by_id_lock_.release();
  }
  currentThread->loader_->arch_memory_.unmapPage(((UserThread*)currentThread)->virtual_page_);
  current_process.threads_lock_.release();
  debug(SYSCALL, "Syscall::PTHREAD_EXIT_UNLOCK\n");
  debug(SYSCALL, "PTHREAD_EXIT: Thread %ld gets killed. \n",currentThread->getTID());
  currentThread->kill();  //TODO: i think this is fine, since is no longer on the list
  assert(false && "This should never happen");
}





int Syscall::pthread_cancel(size_t thread_id) //probably broken
{
  debug(SYSCALL, "Syscall::PTHREAD_CANCEL: called with thread_id %ld.\n",thread_id);
  UserProcess& current_process = *currentThread->process_;
  UserThread* thread_to_be_canceled;
  current_process.threads_lock_.acquire();
  debug(SYSCALL, "Syscall::PTHREAD_CANCEL LOCK\n");
  thread_to_be_canceled = current_process.get_thread_from_threadlist(thread_id);
  if(!thread_to_be_canceled)
  {
    current_process.threads_lock_.release();
    return -1;
  }
  thread_to_be_canceled->wants_to_be_canceled_ = true;
  thread_to_be_canceled->cancel_thread_ = currentThread;                  //need to check if currentThread is still valid if using cancel_thread
  current_process.threads_lock_.release();  
   debug(SYSCALL, "Syscall::PTHREAD_CANCEL UNLOCK\n");

  if(((UserThread*)currentThread)->cancel_type_ == PTHREAD_CANCEL_DEFERRED)
  {
    currentThread->has_reached_cancelation_point_lock_.acquire();
    while(!currentThread->reached_cancelation_point_)
    {
      currentThread->has_reached_cancelation_point_.wait();
    }
    currentThread->has_reached_cancelation_point_lock_.release();
  }
  else
  {
    current_process.threads_lock_.acquire();
    if(thread_to_be_canceled->switch_to_userspace_ == 1)
    {
      ustl::vector<UserThread*>::iterator iterator = ustl::find(current_process.threads_.begin(), current_process.threads_.end(), thread_to_be_canceled);
      current_process.threads_.erase(iterator);
      thread_to_be_canceled->kill();             //do the pthread_exit stuff
      current_process.threads_lock_.release();
    }
    else
    {
      current_process.threads_lock_.release();
      currentThread->has_reached_cancelation_point_lock_.acquire();
      while(!currentThread->reached_cancelation_point_)
      {
        currentThread->has_reached_cancelation_point_.wait();
      }
      currentThread->has_reached_cancelation_point_lock_.release();
    }
    
  }


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

int Syscall::pthread_create(size_t* thread, unsigned int* attr, void *(*start_routine)(void*), void* arg, void *(*wrapper_address)())         
{
  UserProcess& current_process = *currentThread->process_;
  if(!(check_parameter((size_t)thread) && check_parameter((size_t)attr, true) && check_parameter((size_t)start_routine) && check_parameter((size_t)arg, true) && check_parameter((size_t)wrapper_address)))
  {
    return -1;
  }
  debug(SYSCALL, "Unused: Thread %p, Attribute %p\n", thread, attr);       //TODO
  int rv = current_process.create_thread(thread, start_routine, wrapper_address, arg);
  return rv;
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
  debug(SYSCALL, "Currently unused %ld and %ld\n", (size_t)path, (size_t)argv);
  UserProcess& current_process = *currentThread->process_;
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

  //destroy other threads
  ((UserThread*)currentThread)->last_thread_before_exec_ = true;
  ((UserThread*)currentThread)->kill();

  
  assert(0 && "Sucessful exec should not return");
}


 void Syscall::send_cancelation_notification()
 {
    UserProcess& current_process = *currentThread->process_;
    if(!current_process.check_if_thread_in_threadList((UserThread*)((UserThread*)currentThread)->cancel_thread_))         //not sure if that can even happen
    {
      return;
    }
    currentThread->cancel_thread_->has_reached_cancelation_point_lock_.acquire();
    currentThread->cancel_thread_->reached_cancelation_point_ = true;
    currentThread->cancel_thread_->has_reached_cancelation_point_.signal();
    currentThread->cancel_thread_->has_reached_cancelation_point_lock_.release();
 }

int Syscall::pthread_setcancelstate(int state, int *oldstate)
{
  if(state != 0 && state != 1)         //what if another enum corresponding to 1
  {
    return -1;
  }
  *oldstate = (int)((UserThread*)currentThread)->cancel_state_;              //TODO: should be atomic
  ((UserThread*)currentThread)->cancel_state_ = (CANCEL_STATE)state;
  return 0;
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
  if(type != 0 && type != 1)         //what if another enum corresponding to 1
  {
    return -1;
  }
  *oldtype = (int)((UserThread*)currentThread)->cancel_type_;             //TODO: should be atomic
  ((UserThread*)currentThread)->cancel_type_ = (CANCEL_TYPE)type;
  
  return 0;
}