#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
    : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info),
      filename_(filename), terminal_number_(terminal_number) {
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           // This belong to Thread, not sure what to do here
    return;
  }
  size_t new_tid = generateUniqueTid();
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, new_tid, NULL, NULL, (size_t)new_tid));
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

size_t getTID();


size_t UserProcess::generateUniqueTid() {
  static size_t last_tid = 0;
  return ++last_tid;
}


UserThread* UserProcess::createUserThread(const ThreadCreateParams& params) {
  size_t new_tid = generateUniqueTid();
  auto new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, new_tid, params.startRoutine, params.arg, (size_t)new_tid);
  threads_.push_back(new_thread);
  Scheduler::instance()->addNewThread(new_thread);
  return 0;
}
