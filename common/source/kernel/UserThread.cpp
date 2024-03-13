#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "PageManager.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, size_t tid)
            :Thread(working_dir, name, type, loader), process_(process)
{
    tid_ = tid;

    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;
}

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number,
                       Loader* loader, UserProcess* process, size_t tid, void *(*func)(void*), void* arg)
            : Thread(working_dir, name, type, loader), process_(process)
{
    tid_ = tid;

    ArchThreads::createUserRegisters(user_registers_, (void*) func,
                                     (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * tid),
                                     getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    user_registers_->rdi = (size_t)arg;
    user_registers_->rsi = (size_t)arg;

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