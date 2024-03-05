#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process):Thread(working_dir, name, type, loader), process_(process)
{
    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);   

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;
}
UserThread::~UserThread()
{
    //delete process_;
    //process_->to_be_destroyed_ = true;
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


// DO NOT use new / delete in this Method, as it is sometimes called from an Interrupt Handler with Interrupts disabled
void UserThread::kill()
{
  debug(USERTHREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "This should never happen, how are we still alive?");
  }
}