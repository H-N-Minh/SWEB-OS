#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) 
    : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), tid_counter_(1)
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
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, tid_counter_));
  tid_counter_++;
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

void UserProcess::createUserThread(void* func, void* para)
{
  debug(MINH, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
  // threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, ((UserThread*) currentThread)->process_));
}

