#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "umap.h"
#include "UserThread.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info)
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

  // not sure this belong to thread or process
  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this));
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

// void UserProcess::Run()
// {
//   debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
//   assert(false);
// }


// Setter and Getter for Terminal and WorkingDirInfo
void UserProcess::setTerminal(Terminal *my_term)
{
  my_terminal_ = my_term;
}

Terminal *UserProcess::getTerminal()
{
  return my_terminal_ ? my_terminal_ : main_console->getActiveTerminal();
}

FileSystemInfo* UserProcess::getWorkingDirInfo()
{
  return working_dir_;
}

void UserProcess::setWorkingDirInfo(FileSystemInfo* working_dir)
{
  working_dir_ = working_dir;
}
