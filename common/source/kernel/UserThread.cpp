#include "UserThread.h"

#include "kprintf.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Console.h"
#include "Terminal.h"
#include "KernelMemoryManager.h"
#include "Stabs2DebugInfo.h"

#define BACKTRACE_MAX_FRAMES 20
class UserThread;


UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name)
        : Thread(working_dir, ustl::move(name), Thread::USER_THREAD) {
    switch_to_userspace_ = 1;

    debug(THREAD, "Thread ctor, this is %p, stack is %p, fs_info ptr: %p\n", this, kernel_stack_, working_dir_);

    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                     (void*) (USER_BREAK - sizeof(pointer)),
                                     getKernelStackStartPointer());

    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    kernel_stack_[2047] = STACK_CANARY;
    kernel_stack_[0] = STACK_CANARY;
}

UserThread::~UserThread()
{
    debug(THREAD, "~Thread: freeing ThreadInfos\n");
    delete user_registers_;
    user_registers_ = 0;
    delete kernel_registers_;
    kernel_registers_ = 0;
    if(unlikely(holding_lock_list_ != 0))
    {
        debug(THREAD, "~Thread: ERROR: Thread <%s (%p)> is going to be destroyed, but still holds some locks!\n",
              getName(), this);
        Lock::printHoldingList(this);
        assert(false);
    }
    debug(THREAD, "~Thread: done (%s)\n", name_.c_str());
}



FileSystemInfo* getcwd()
{
    if (FileSystemInfo* info = currentThread->getWorkingDirInfo())
        return info;
    return default_working_dir;
}


extern Stabs2DebugInfo const *kernel_debug_info;


