#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

#include "ArchInterrupts.h"
#include "types.h"

int32 UserProcess::tid_counter_ = 1;
int32 UserProcess::pid_counter_ = 1;
//todo: use ArchThreads::atomic_add

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
  : fd_(VfsSyscall::open(other.filename_, O_RDONLY)), working_dir_(other.working_dir_), filename_(other.filename_), terminal_number_(other.terminal_number_), 
    loader_(0), pid_(pid_counter_++)
{
  debug(USERPROCESS, "Copy-ctor: start copying from process (%u) \n", other.pid_);
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  assert(fd_ >= 0  && "Error: File descriptor doesnt exist, Loading failed in UserProcess copy-ctor\n");
  debug(USERPROCESS, "Copy-ctor: Calling Archmemory copy-ctor for new Loader\n");
  loader_ = new Loader(fd_, other.loader_->arch_memory_);

  debug(USERPROCESS, "Copy-ctor: Done loading with new ArchMemory, now calling copy-ctor for Thread\n");
  UserThread* curr_thread = (UserThread*) currentThread;  
  UserThread* new_thread = new UserThread(*curr_thread, this, tid_counter_, terminal_number_, loader_);
  threads_.push_back(new_thread);

  // DEBUGGING, can be deleted
  // new_thread->loader_->arch_memory_.checkAddressValid((pointer) 0xfffff00000400000);
  // new_thread->loader_->arch_memory_.checkAddressValid(0xfffff00000401000);
  // new_thread->loader_->arch_memory_.checkAddressValid(0xfffff00000402000);
  // new_thread->loader_->arch_memory_.checkAddressValid(0xfffff00000403000);
  // new_thread->loader_->arch_memory_.checkAddressValid(0xfffff00000407000);

  // currentThread->loader_->arch_memory_.checkAddressValid(0xfffff00000400000);
  // currentThread->loader_->arch_memory_.checkAddressValid(0xfffff00000401000);
  // currentThread->loader_->arch_memory_.checkAddressValid(0xfffff00000402000);
  // currentThread->loader_->arch_memory_.checkAddressValid(0xfffff00000403000);
  // currentThread->loader_->arch_memory_.checkAddressValid(0xfffff00000407000);

  debug(USERPROCESS, "Copy-ctor: Done copying Thread, adding new thread id (%zu) to the Scheduler", new_thread->getTID());
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

