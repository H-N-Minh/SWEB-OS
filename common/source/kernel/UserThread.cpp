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

    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");

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
    debug(TAI_THREAD, "tid vAlue %zu \n", tid);
    debug(TAI_THREAD, "------------------------------------func in userthread is %p\n", func);

    size_t page_for_stack = PageManager::instance()->allocPPN();
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - tid, page_for_stack, 1);
    assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
    ArchThreads::createUserRegisters(user_registers_, (void*) func,

                                     (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * (tid-1)),
                                     getKernelStackStartPointer());
    user_registers_->rdi = (size_t)arg;

    debug(TAI_THREAD, "----------------------para value %zu \n", (size_t)arg);

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);
    debug(TAI_THREAD, "go here3 \n");
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    switch_to_userspace_ = 1;

    debug(TAI_THREAD, "go here4 \n");
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