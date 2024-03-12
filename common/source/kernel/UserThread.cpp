#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"
#include "ArchInterrupts.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, 
                        Loader* loader, UserProcess* process, int32 tid, void* func, void* para, void* pcreate_helper)
    : Thread(working_dir, name, type, loader), process_(process)
{
    debug(USERTHREAD, "UserThread Constructor: creating new thread with func (%p), para (%zu) \n", func, (size_t) para);
    tid_ = tid;

    // allocate physical page for stack and map to virtual memory
    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - tid, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    // Setting up Registers
    void* start_func_ptr;
    void* user_stack_ptr = (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * (tid-1));
    if (!func)
    {
        // create first thread of process => start the "main" func
        start_func_ptr = loader_->getEntryFunction();
    }
    else
    {
        // Create another thread for a process
        start_func_ptr = pcreate_helper;
    }
    ArchThreads::createUserRegisters(user_registers_, start_func_ptr, user_stack_ptr, getKernelStackStartPointer());
    if (func)
    {
        user_registers_->rdi = (size_t)func;
        user_registers_->rsi = (size_t)para;
    }

    // Setting up AddressSpace and Terminal
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);   
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;
}

// DO NOT use new / delete in this Method, as it is sometimes called from an Interrupt Handler with Interrupts disabled
void Thread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

  // remove itself from list of threads in process
  ustl::vector<UserThread*> threads_ = ((UserThread*) currentThread)->process_->threads_;
  auto thread = ustd::find(threads_.begin(), threads_.end(), currentThread);
  if (thread != threads_.end()) {
      threads_.erase(thread);
  }
  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "This should never happen, how are we still alive?");
  }
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
