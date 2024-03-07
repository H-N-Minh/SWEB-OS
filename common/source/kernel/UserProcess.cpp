#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), terminal_number_(terminal_number), filename_(filename)
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
  
  thread_counter_++;
  UserThread* new_thread = new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0, thread_counter_);
  addThreadtoThreadList(new_thread);    
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  //ProcessRegistry::instance()->processExit();
}

int UserProcess::create_thread(size_t* thread, void *(*start_routine)(void*), void *(*wrapper)(), void* arg)
{
  thread_counter_++;
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, start_routine, wrapper, arg, thread_counter_);
  if(new_thread)
  {
    addThreadtoThreadList(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    *thread = new_thread->getTID();
    return 0;
  }
  else
  {
    return -1;
  }
  
}

ustl::vector<UserThread*> UserProcess::getThreads(){
  return threads_;
}

void UserProcess::addThreadtoThreadList(UserThread* thread){
  threads_.push_back(thread);
}



