#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "UserProcess.h"

#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "types.h"
#include "Syscall.h"

int64 UserProcess::tid_counter_ = 1;
int64 UserProcess::pid_counter_ = 1;


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
  : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number),
    threads_lock_("thread_lock_"),  execv_lock_(" execv_lock_")
{
  ProcessRegistry::instance()->processStart();
  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    // TODO: clean process when no loader (kill();)
    return;
  }
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0, false));
  debug(USERPROCESS, "ctor: Done creating Thread\n");
}


// COPY CONSTRUCTOR
UserProcess::UserProcess(const UserProcess& other)
  : fd_(VfsSyscall::open(other.filename_, O_RDONLY)), working_dir_(new FileSystemInfo(*other.working_dir_)), filename_(other.filename_), 
    terminal_number_(other.terminal_number_), threads_lock_("thread_lock_"), execv_lock_(" execv_lock_")
{
  debug(FORK, "Copy-ctor UserProcess: start copying from process (pid:%u) \n", other.pid_);
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  loader_ = new Loader(*other.loader_, fd_);
  if (!loader_){assert(0 && "No loader in fork");}

  UserThread* child_thread = new UserThread(*(UserThread*) currentThread, this);
  threads_.push_back(child_thread);

  debug(USERPROCESS, "Copy-ctor: Done copying Thread, adding new thread id (%zu) to the Scheduler", child_thread->getTID());
  Scheduler::instance()->addNewThread(child_thread);
}


UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  debug(USERPROCESS, "Delete loader %p from process %d.\n", loader_, pid_);
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

UserThread* UserProcess::getUserThread(size_t tid)
{
  for (size_t i = 0; i < threads_.size(); i++)
  {
    if (threads_[i]->getTID() == tid)
      return threads_[i];
  }
  return 0;
}


int UserProcess::removeRetvalFromMapAndSetReval(size_t tid, void**value_ptr)
{
  //Todo: assert that it holds thread lock
  ustl::map<size_t, void*>::iterator iterator = thread_retval_map_.find(tid);                                                   
  if(iterator != thread_retval_map_.end())
  {
    void *return_value = thread_retval_map_[tid];
    thread_retval_map_.erase(iterator);
    if(value_ptr != NULL)
    {
      *value_ptr = return_value;
    }
    return 0;
  }
  else
  {
    return -1;
  }
}


bool UserProcess::isThreadInVector(UserThread* test_thread)
{
  //TODO: check if lock is held
  for (auto& thread : threads_)
  {
    if(test_thread == thread)
    {
      return true;
    } 
  }
  return false;
}

int UserProcess::createThread(size_t* thread, void* start_routine, void* wrapper, void* arg)
{
  debug(USERPROCESS, "UserProcess::createThread: func (%p), para (%zu) \n", start_routine, (size_t) arg);

  threads_lock_.acquire();  
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, start_routine, 
                                          arg, wrapper, false);
  if(new_thread)
  {
    debug(USERPROCESS, "UserProcess::createThread: Adding new thread to scheduler\n");
    threads_.push_back(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    *thread = new_thread->getTID();
    threads_lock_.release();  
    return 0;
  }
  else
  {
    debug(USERPROCESS, "UserProcess::createThread: ERROR: Thread not created\n");
    threads_lock_.release();  
    return -1;
  }
}

int UserProcess::joinThread(size_t thread_id, void**value_ptr)
{
  debug(SYSCALL, "UserProcess:joinThread: called, thread_id: %zu and %p\n", thread_id, value_ptr);
  UserThread& currentUserThread = *((UserThread*)currentThread);

  threads_lock_.acquire();

  //find thread in threadlist
  UserThread* thread_to_be_joined = getUserThread(thread_id);
  if(!thread_to_be_joined)
  {
    //Check if thread has already terminated
    int thread_in_retval_map = removeRetvalFromMapAndSetReval(thread_id, value_ptr);
    threads_lock_.release();
    return thread_in_retval_map;
  }
  currentUserThread.thread_gets_killed_lock_.acquire();
  thread_to_be_joined->join_threads_.push_back(&currentUserThread);
  threads_lock_.release();
  
  //wait for thread get killed
  while(!currentUserThread.thread_killed)
  {
    currentUserThread.thread_gets_killed_.wait();
  }     
  currentUserThread.thread_killed = false;               
  currentUserThread.thread_gets_killed_lock_.release(); 

  threads_lock_.acquire();
  int thread_in_retval_map = removeRetvalFromMapAndSetReval(thread_id, value_ptr);
  threads_lock_.release();

  return thread_in_retval_map;
}


int UserProcess::execvProcess(const char *path, char *const argv[])
{
  execv_lock_.acquire();
  execv_fd_ = VfsSyscall::open(path, O_RDONLY);
  if (execv_fd_ >= 0)
  {
    execv_loader_ = new Loader(execv_fd_);
  }
  else
  {
    //Error no file descriptor
    execv_lock_.release();
    return -1;
  }

  //Error no loader
  if (!execv_loader_ || !execv_loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", path);
    delete execv_loader_;
    execv_loader_ = 0;
    VfsSyscall::close(execv_fd_);
    execv_lock_.release();
    return -1;
  }

  //map arguments to identity mapping
  execv_ppn_args_ = PageManager::instance()->allocPPN();
  size_t virtual_address =  ArchMemory::getIdentAddressOfPPN(execv_ppn_args_);

  size_t array_offset = 3500; //choose a good value

  size_t index = 0;
  size_t offset = 0;
  while(1)
  {
    if(argv[index] == NULL)
    {
      break;
    }
    if(!Syscall::check_parameter((size_t)argv[index], true) || (offset +  strlen(argv[index]) + 1) >= array_offset || index >= 300)
    {
      delete execv_loader_;
      execv_loader_ = 0;
      VfsSyscall::close(execv_fd_);
      PageManager::instance()->freePPN(execv_ppn_args_);
      execv_lock_.release();
      return -1;
    }

    memcpy((char*)virtual_address + offset, argv[index], strlen(argv[index])+1);
    memcpy((void*)(virtual_address + array_offset + index * sizeof(pointer)), &offset, sizeof(pointer));

    offset += strlen(argv[index]) + 1;

    index++;
  }
  exec_argc_ = index;

  Syscall::exit(0, true);
  
  while(1)            //TODO should not be busy wait
  {
    threads_lock_.acquire();
    if(threads_.size() == 1)
    {
      threads_lock_.release();
      break;
    }
    threads_lock_.release();
  }

  execv_lock_.release();
  ustl::vector<UserThread*>::iterator exiting_thread = ustl::find(threads_.begin(), threads_.end(), currentThread);
  threads_.erase(exiting_thread);
  ((UserThread*)currentThread)->last_thread_before_exec_ = true;
  ((UserThread*)currentThread)->kill();

  
  assert(0 && "Sucessful exec should not return");

}
