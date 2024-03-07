#include "UserThread.h"


#include "ArchThreads.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Console.h"
#include "Terminal.h"
#include "Stabs2DebugInfo.h"

class UserThread;


UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Loader* loader)
        : Thread(working_dir, name), loader_(loader) {

    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                     (void*) (USER_BREAK - sizeof(pointer)),
                                     getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    switch_to_userspace_ = 1;
}



UserThread::~UserThread() {
    //empty
}

void UserThread::Run() {
    //empty
}