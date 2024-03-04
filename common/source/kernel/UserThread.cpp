#include "UserProcess.h"
#include "UserThread.h"
#include "Thread.h"
#include "Console.h"
#include "ArchThreads.h"
#include "Loader.h"
#include "debug.h"


UserThread::UserThread(FileSystemInfo* working_dir, ustl::string name, Thread::TYPE type, Loader* loader, int32 terminal_number, UserProcess* process, void *(*function_start)(void*), void* function_args)  
                                        :Thread(working_dir, name, type, loader), process_{process}
{
    if(function_start == 0)     //this is definitely not how we should check this
    {
        ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(), (void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
        ArchThreads::setAddressSpace(this, loader_->arch_memory_);
        
        if (main_console->getTerminal(terminal_number))
        {   
            setTerminal(main_console->getTerminal(terminal_number));
        }
        debug(USERPROCESS, "Unused: Function_start %p, function_args %p", function_start, function_args);
        switch_to_userspace_ = 1;
         //debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
    }
    else
    {
        ArchThreads::createUserRegisters(user_registers_, (void*)function_start, (void*) (USER_BREAK - sizeof(pointer)), getKernelStackStartPointer());
        ArchThreads::setAddressSpace(this, loader_->arch_memory_);
        
        if (main_console->getTerminal(terminal_number))
        {   
            setTerminal(main_console->getTerminal(terminal_number));
        }
        debug(USERPROCESS, "Unused: Function_start %p, function_args %p", function_start, function_args);
        switch_to_userspace_ = 1;
    }
    





    

}

UserThread::~UserThread()
{
    debug(USERPROCESS, "Number of threads in process %ld\n", process_->threads.size());
    // if(process_->threads.size() == 1)
    // {
        delete process_;              //should only be called for the last thread (and i am not sure, if userthread is the right place)
    //}
        
}

UserProcess* UserThread::get_process()
{
    return process_;
}

