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

UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, uint32 terminal_number, Loader* loader, UserProcess* process, 
            void *(*start_routine)(void*), void *(*wrapper)(), void* arg, size_t thread_counter):Thread(working_dir, name, type, loader)
{
    process_ = process;

    size_t page_for_stack = PageManager::instance()->allocPPN();
    size_t user_stack_start;

    if(wrapper == 0)
    {
        virtual_page_ = USER_BREAK / PAGE_SIZE - 1;
        user_stack_start = USER_BREAK - sizeof(pointer);
        bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page_, page_for_stack, 1);
        assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),(void*)user_stack_start, getKernelStackStartPointer());
        debug(USERTHREAD, "First thread: Stack starts at %zd(=%zx) and virtual page is%zd(=%zx)\n\n",user_stack_start, user_stack_start, virtual_page_, virtual_page_);
    }
    else
    {
        virtual_page_ = USER_BREAK / PAGE_SIZE - 1 - thread_counter;
        user_stack_start = USER_BREAK - sizeof(pointer) - PAGE_SIZE * thread_counter;
        bool vpn_mapped = loader_->arch_memory_.mapPage(virtual_page_ , page_for_stack, 1);
        assert(vpn_mapped && "Virtual page for stack was already mapped - this should never happen");
        ArchThreads::createUserRegisters(user_registers_, (void*)wrapper, (void*)user_stack_start, getKernelStackStartPointer());
        user_registers_->rdi = (size_t)start_routine;
        user_registers_->rsi = (size_t)arg;
        debug(USERTHREAD, "Pthread_create: Stack starts at %zd(=%zx) and virtual page is %zd(=%zx)\n\n",user_stack_start, user_stack_start, virtual_page_, virtual_page_);

    }

    

    ArchThreads::setAddressSpace(this, loader_->arch_memory_); 
    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    setTID(thread_counter);

    switch_to_userspace_ = 1;            //maybe add to userprocess after adding thread to threads_


}
UserThread::~UserThread()
{
    if(last_thread_alive_)
    {
        debug(USERTHREAD, "Userprocess gets deleted\n");
        delete process_;
        process_ = 0;
    }
    if(loader_->arch_memory_.checkAddressValid(virtual_page_))
    {
        loader_->arch_memory_.unmapPage(virtual_page_);
    }
}

void UserThread::Run(){
    assert(0);
}
