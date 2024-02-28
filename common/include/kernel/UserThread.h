#pragma once

#include "Thread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"

class UserThread : public Thread{
    public:
        UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number):Thread(working_dir, name, type, loader)
        {
            ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                   (void*) (USER_BREAK - sizeof(pointer)),
                                   getKernelStackStartPointer());

            ArchThreads::setAddressSpace(this, loader_->arch_memory_);

            //debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

            if (main_console->getTerminal(terminal_number))
                setTerminal(main_console->getTerminal(terminal_number));

            switch_to_userspace_ = 1;
        }
};