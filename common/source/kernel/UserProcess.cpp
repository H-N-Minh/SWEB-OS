#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

#include "ArchInterrupts.h"

int32 UserProcess::tid_counter_ = 1;
int32 UserProcess::pid_counter_ = 1;

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) 
    : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), 
      filename_(filename), terminal_number_(terminal_number), loader_(0), pid_(pid_counter_++)
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           // This belong to Thread, not sure what to do here
    return;
  }
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 
                                    tid_counter_, NULL, NULL, NULL));
  tid_counter_++;
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::UserProcess(const UserProcess& other)
  : fd_(other.fd_), working_dir_(other.working_dir_), filename_(other.filename_), terminal_number_(other.terminal_number_), 
    loader_(other.loader_), pid_(pid_counter_++)
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  debug(USERPROCESS, "Copy-ctor: Creating new thread for forked process");

  UserThread* curr_thread = (UserThread*) currentThread;  
  UserThread* new_thread = new UserThread(*curr_thread, this, tid_counter_, terminal_number_);
  threads_.push_back(new_thread);

  debug(USERPROCESS, "ctor: Done loading %s, now adding new thread id (%zu) to the Scheduler\n", filename_.c_str(), new_thread->getTID());
  Scheduler::instance()->addNewThread(new_thread);
  tid_counter_++;
}


UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

void UserProcess::createUserThread(void* func, void* para, void* tid, void* pcreate_helper)
{
  debug(USERPROCESS, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, 
                                          ((UserThread*) currentThread)->process_, tid_counter_, func, para, pcreate_helper);
  threads_.push_back(new_thread);
  *((unsigned long*) tid) = (unsigned long) tid_counter_;
  tid_counter_++;

  debug(USERPROCESS, "UserProcess::createUserThread: Adding new thread to scheduler\n");
  Scheduler::instance()->addNewThread(new_thread);
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

