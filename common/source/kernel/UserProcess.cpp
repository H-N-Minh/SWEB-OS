#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info)
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


size_t UserProcess::generateUniqueTid() {
  static size_t last_tid = 0;
  return ++last_tid;
}


void UserProcess::createUserThread(const ThreadCreateParams& params, size_t* tid_address) {
  debug(Fabi, "createUserThread: startRoutine (%p), arg (%p)\n", params.startRoutine, params.arg);

  size_t new_tid = generateUniqueTid();

  UserThread* new_thread = new UserThread(working_dir_, filname_, Thread::USER_THREAD, terminal_number_, loader_, this, new_tid, params.startRoutine, params.arg);

  threads_.push_back(new_thread);

  if (tid_address != nullptr) {
    *tid_address = new_tid;
  }

  debug(Fabi, "createUserThread: New thread ID is %zu\n", new_tid);

  Scheduler::instance()->addNewThread(new_thread);
}
