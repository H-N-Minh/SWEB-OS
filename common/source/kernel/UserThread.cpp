#include "UserProcess.h"
#include "UserThread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"
#include "Scheduler.h"
#include "ProcessRegistry.h"
#include "debug.h"
#include "PageManager.h"

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter):Thread(working_dir, name, type, loader), process_(process)
{
    size_t page_for_stack = PageManager::instance()->allocPPN();
    if(wrapper == 0)
    {
        bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);
        assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    }
    else
    {
        bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1 - thread_counter , page_for_stack, 1); //idk
        assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
        ArchThreads::createUserRegisters(user_registers_, (void*)wrapper, (void*) (USER_BREAK - sizeof(pointer) - PAGE_SIZE * thread_counter), getKernelStackStartPointer());
        user_registers_->rdi = (size_t)start_routine;
        user_registers_->rsi = (size_t)arg;

    }

    ArchThreads::setAddressSpace(this, loader_->arch_memory_); 
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    setTID(thread_counter);

    switch_to_userspace_ = 1;            //maybe add to userprocess after adding thread to threads_


}
UserThread::~UserThread()
{
    if(!process_)
    {
        assert(Scheduler::instance()->isCurrentlyCleaningUp());
        delete loader_;
        loader_ = 0;
        ProcessRegistry::instance()->processExit();
    }
    
}

void UserThread::Run(){
    assert(0);
}

UserProcess* UserThread::getProcess(){
    return process_;
}


void UserThread::setProcess(UserProcess* process){
    process_ = process;
}