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
#include "UserThread.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
        : fd_(VfsSyscall::open(filename, O_RDONLY)), terminal_number_(terminal_number), filename_(filename), working_dir_(fs_info), loader_(nullptr) {
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  // new error
    if (fd_ < 0) {
        debug(USERPROCESS, "Error: Could not open file %s.\n", filename.c_str());
        return;
    }
    loader_ = new Loader(fd_);
    //


  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    return;
  }

  size_t page_for_stack = PageManager::instance()->allocPPN();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
  assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    createInitialThread();

  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));

}

UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
    for (auto &thread : threads) {
        delete thread;
    }
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}


void UserProcess::createInitialThread() {
    auto *initialThread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, loader_, terminal_number_, this);
    threads.push_back(initialThread);
    Scheduler::instance()->addNewThread(initialThread);
}



