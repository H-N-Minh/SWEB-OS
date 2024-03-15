#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"

UserThread::UserThread(FileSystemInfo* working_dir, const ustl::string& name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, size_t tid, void* startRoutine, void* arg, size_t counter)
    : Thread(working_dir, name, type, loader), process_(process), tid_(tid){

  debug(Fabi, "UserThread erstellt: TID=%zu, startRoutine=%p, arg=%p", tid, startRoutine, arg);

  size_t page_for_stack = PageManager::instance()->allocPPN();
  void* user_stack_pointer = (void*)(USER_BREAK - sizeof(pointer ) - PAGE_SIZE * (counter -1));
  bool mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - counter, page_for_stack, 1);
  assert(mapped && "Mapping the user stack page failed");


  debug(Fabi, "-------------------------UserThread erstellt: TID=%p, startRoutine=%p", startRoutine, loader_->getEntryFunction());
  ArchThreads::createUserRegisters(user_registers_, startRoutine ? startRoutine : loader_->getEntryFunction(), (void*)user_stack_pointer, getKernelStackStartPointer());

  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number)) {
    setTerminal(main_console->getTerminal(terminal_number));
  }
  debug(Fabi, "passed4");
  //assert(0 && "Cdrlasöjkres");
  setTID(counter);

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

//atm rsp rsp0 mismatch kernel stack pointer and stack pointer for entering userspace -> Kernel panic
//things to look at: tid_ in UserThread
//stack allocation and setup
//User and kernel register init
//maybe syscall handling
//
