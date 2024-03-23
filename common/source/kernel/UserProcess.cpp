#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "UserProcess.h"

size_t UserProcess::pid_counter_ = 0;

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) : fd_(VfsSyscall::open(filename, O_RDONLY)), 
        working_dir_(fs_info), terminal_number_(terminal_number), filename_(filename), thread_counter_lock_("thread_counter_lock_"),
        threads_lock_("thread_lock_"), value_ptr_by_id_lock_("value_ptr_by_id_lock_")
        
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           //TODO
    return;
  }
  thread_counter_++;            //should be fine without locking since we are still singlethreaded
  UserThread* new_thread = new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0, thread_counter_, false);
  threads_.push_back(new_thread); 
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  pid_counter_++;         //Todo:locking
  pid_ = pid_counter_;
}

UserProcess::UserProcess(UserProcess const &src):    
    fd_(src.fd_), working_dir_(src.working_dir_), terminal_number_(src.terminal_number_), filename_(src.filename_), thread_counter_(0),
    thread_counter_lock_("thread_counter_lock_"), threads_lock_("thread_lock_"), value_ptr_by_id_lock_("value_ptr_by_id_lock_"),
    execv_loader_(0), execv_fd_(0), execv_ppn_args_(0), exec_argc_(0), parent_pid_(src.pid_)
{
  debug(FORK, "Copy constructor UserProcess\n");

  ProcessRegistry::instance()->processStart();

  pid_counter_++;          //Todo:locking
  pid_ = pid_counter_;

  loader_ = new Loader(*src.loader_);
  // if (!loader_ || !loader_->loadExecutableAndInitProcess())
  // {
  //   assert(0 && "This would be bad.");
  // }

  thread_counter_++;
  UserThread& currentUserThread = *(UserThread*)currentThread;
  UserThread* new_thread = new UserThread(currentUserThread, this, thread_counter_);
  threads_.push_back(new_thread); 
  
 

  //working_dir
  //fd

  //filename
  //terminal_number
  //thread_counter




}


UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

int UserProcess::create_thread(size_t* thread, void *(*start_routine)(void*), void *(*wrapper)(), void* arg)
{
  thread_counter_lock_.acquire();
  thread_counter_++;
  threads_lock_.acquire();  
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, start_routine, wrapper, arg, thread_counter_, false);
  thread_counter_lock_.release();
  if(new_thread)
  {
    threads_.push_back(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    *thread = new_thread->getTID();
    threads_lock_.release();  
    return 0;
  }
  else
  {
    threads_lock_.release();  
    return -1;
  }
}


UserThread* UserProcess::get_thread_from_threadlist(size_t id)
{
  for (auto& thread : threads_)
  {
    if(id == thread->getTID())
    {
      return thread;
    } 
  }
  return NULL;
}

bool UserProcess::check_if_thread_in_threadList(UserThread* test_thread)
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





