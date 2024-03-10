#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"

// create first thread of process
UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, int32 tid)
    : Thread(working_dir, name, type, loader), process_(process)
{
    tid_ = tid;
    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);   

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;
}

// Create another thread for a process
UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, 
                        Loader* loader, UserProcess* process, int32 tid, void* func, void* para)
    : Thread(working_dir, name, type, loader), process_(process)
{
    tid_ = tid;
    debug(MINH, "UserThread::UserThread: func (%p), para (%zu) \n", func, (size_t) para);
    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1 - tid, page_for_stack, 1);       /////
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

    ArchThreads::createUserRegisters(user_registers_, (void*) func,(void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * tid), getKernelStackStartPointer());
    user_registers_->rdi = (size_t)para;
    user_registers_->rsi = (size_t)para;        ///////////

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
