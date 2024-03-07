#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "Console.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "ArchThreads.h"
#include "offsets.h"
#include "Scheduler.h"

#include "Loader.h"

//initializes a UserProcess object, and it will initialize the base class Thread
UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number): fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number)
//:
//    Thread(fs_info, filename, Thread::USER_THREAD), fd_(VfsSyscall::open(filename, O_RDONLY))
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    return;
  }

  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

//  delete working_dir_;
//  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

void UserProcess::Run()
{
    Thread* thread1 = createThread("Thread1");
    Thread* thread2 = createThread("Thread2");

    terminateThread(thread1);
    terminateThread(thread2);
    debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
    assert(false);
}

Thread* UserProcess::createThread(ustl::string thread_name) {
    Thread* new_thread = new UserThread(working_dir_, thread_name, Thread::USER_THREAD, loader_, 0);
    threads_.push_back(new_thread);
    return new_thread;
}

void UserProcess::terminateThread(Thread* thread) {
    auto it = ustl::find(threads_.begin(), threads_.end(), thread);
    if (it != threads_.end()) {
        threads_.erase(it);
        delete thread;
    }
}

ustl::vector<Thread *> UserProcess::getThread() {
    return threads_;
}

