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
    //kill();           //!
    //to_be_destroyed_ = true;     //123
    return;
  }

  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0));       //zero zero
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

int UserProcess::add_thread(void *(*start_routine)(void*), void *(*wrapper)())
{
  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1 - (this->threads_.size()) , page_for_stack, 1);  //TODO -> only works if no deletion and idk??
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
  
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, start_routine, wrapper);  //TODO ->not sure if filname and working_dir are actually thread specific
  if(new_thread)
  {
    threads_.push_back(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    return 0;
  }
  else
  {
    return -1; //error
  }
  
}


// void UserProcess::Run()
// {
//   debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
//   assert(false);
// }

