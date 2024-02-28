#include "UserProcess.h"
#include "UserThread.h"
#include "Thread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"


UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number, UserProcess* process):Thread(working_dir, name, type, loader), process_{process}
{
    ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(), (void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    //debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

    if (main_console->getTerminal(terminal_number))
    {
        setTerminal(main_console->getTerminal(terminal_number));
    }
                
    switch_to_userspace_ = 1;

}

UserThread::~UserThread()
{
    delete process_;              //should only be called (and i am not sure, if userthread is the right place)
}

