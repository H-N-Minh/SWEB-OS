#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"

UserThread::UserThread(FileSystemInfo* working_dir, const ustl::string& name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, size_t tid, void* startRoutine, void* arg)
    : Thread(working_dir, name, type, loader), process_(process)
{

    this->tid_ = tid;

  ArchThreads::createUserRegisters(user_registers_, startRoutine, arg, getKernelStackStartPointer());
  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));

  switch_to_userspace_ = 1;
}

UserThread::UserThread(FileSystemInfo* working_dir, const ustl::string& name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process)
    : Thread(working_dir, name, type, loader), process_(process) {

  ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));

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
