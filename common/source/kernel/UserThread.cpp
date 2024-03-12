#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"

UserThread::UserThread(FileSystemInfo* working_dir, const ustl::string& name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, size_t tid, void* startRoutine, void* arg)
    : Thread(working_dir, name, type, loader), process_(process), tid_(tid) {

  debug(Fabi, "UserThread erstellt: TID=%zu, startRoutine=%p, arg=%p", tid, startRoutine, arg);

  size_t page_for_stack = PageManager::instance()->allocPPN();
  void* user_stack_pointer = (void*)(USER_BREAK - tid * PAGE_SIZE);
  bool mapped = loader_->arch_memory_.mapPage((size_t)user_stack_pointer / PAGE_SIZE - 1, page_for_stack, true);
  assert(mapped && "Mapping the user stack page failed");

  ArchThreads::createUserRegisters(user_registers_, startRoutine ? startRoutine : loader_->getEntryFunction(), arg, user_stack_pointer);

  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number)) {
    setTerminal(main_console->getTerminal(terminal_number));
  }

  switch_to_userspace_ = 1;
}




UserThread::~UserThread()
{
    if(!process_)
    {
        assert(Scheduler::instance()->isCurrentlyCleaningUp());
        delete loader_;
        loader_ = 0;
    }
}

void UserThread::Run()
{
}
