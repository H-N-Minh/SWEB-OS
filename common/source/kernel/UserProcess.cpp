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

int64 UserProcess::tid_counter_ = 1;
int64 UserProcess::pid_counter_ = 1;


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
  : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number),
    threads_lock_("thread_lock_"), thread_retval_map_lock_("thread_retval_map_lock_")
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           // TODO: clean process when no loader
    return;
  }
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  uint64 tid_counter = ArchThreads::atomic_add(tid_counter_, 1);
  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, tid_counter, 0, 0, 0, false));
  debug(USERPROCESS, "ctor: Done creating Thread\n");
}


// COPY CONSTRUCTOR
UserProcess::UserProcess(const UserProcess& other)
  : fd_(0), working_dir_(new FileSystemInfo(*other.working_dir_)),
    filename_(other.filename_), terminal_number_(other.terminal_number_),
    threads_lock_("thread_lock_"), thread_retval_map_lock_("thread_retval_map_lock_") 
{
  fd_ = VfsSyscall::open("/usr/broski.sweb", O_RDONLY);

  debug(USERPROCESS, "Copy-ctor: start copying from process (%u) \n", other.pid_);
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  assert(fd_ >= 0  && "Error: File descriptor doesnt exist, Loading failed in UserProcess copy-ctor\n");
  debug(USERPROCESS, "Copy-ctor: Calling Archmemory copy-ctor for new Loader\n");
  loader_ = new Loader(fd_, other.loader_->arch_memory_);
  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename_.c_str());
    //kill();           // TODO: clean process when no loader
    return;
  }

  debug(USERPROCESS, "Copy-ctor: Done loading with new ArchMemory, now calling copy-ctor for Thread\n");
  int64 tid_counter = ArchThreads::atomic_add(tid_counter_, 1);
  UserThread* child_thread = new UserThread(*(UserThread*) currentThread, this, tid_counter, terminal_number_, loader_);
  threads_.push_back(child_thread);

  debug(USERPROCESS, "Copy-ctor: Done copying Thread, adding new thread id (%zu) to the Scheduler", child_thread->getTID());
  Scheduler::instance()->addNewThread(child_thread);
}





UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = nullptr;


  /////
  //local_fd_table_.closeAllFileDescriptors();
  /////



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

bool UserProcess::isThreadInVector(UserThread* test_thread)
{
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

  uint64 tid_counter = ArchThreads::atomic_add(tid_counter_, 1);
  threads_lock_.acquire();  
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, tid_counter, start_routine, 
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