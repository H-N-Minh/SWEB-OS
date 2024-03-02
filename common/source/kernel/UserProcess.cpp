#include "UserProcess.h"

#include "UserThread.h"
#include "ProcessRegistry.h"
#include "kprintf.h"
#include "Console.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "ArchThreads.h"
#include "offsets.h"
#include "Scheduler.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) :
    fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), terminal_number_(terminal_number), filename_(filename)
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();                                                                 //???
    return;
  }

  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

  threads.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, loader_, terminal_number, this, 0, 0));    //zeros needs to be changed

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

int UserProcess::add_thread(void *(*start_routine)(void*), void* arg)
{
  //size_t page_for_stack = PageManager::instance()->allocPPN();
  //bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  //assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, loader_, terminal_number_, this, start_routine, arg);
  if(new_thread)
  {
    threads.push_back(new_thread);
    //add to process registery
    return 0;
  }
  else
  {
    return -1;   //error codes to be added
  }
  
}


