#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
            : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), num_thread_(1),
              filename_(filename), terminal_number_(terminal_number)
{
    ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

    if (fd_ >= 0)
        loader_ = new Loader(fd_);

    if (!loader_ || !loader_->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
        //kill();           //!
        //to_be_destroyed_ = true;     //123
        return;
    }

    threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this,
                                      num_thread_, NULL, NULL, NULL));
    num_thread_++;
    debug(TAI_THREAD, "num thread %zu\n", num_thread_);
    debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
    if (fd_ > 0)
        VfsSyscall::close(fd_);

    delete working_dir_;
    working_dir_ = 0;

    ProcessRegistry::instance()->processExit();
}

// void UserProcess::Run()
// {
//   debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
//   assert(false);
// }

void UserProcess::createThread(void* func, void* para, void* tid, void* pcreate_helper)
{
    debug(USERPROCESS, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
    UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_,
                                            ((UserThread*) currentThread)->process_, num_thread_, func, para, pcreate_helper);
    threads_.push_back(new_thread);
    *((unsigned long*) tid) = (unsigned long) num_thread_;
    num_thread_++;

    debug(USERPROCESS, "UserProcess::createUserThread: Adding new thread to scheduler\n");
    Scheduler::instance()->addNewThread(new_thread);
}

UserThread* UserProcess::getUserThread(size_t tid)
{
    for (auto& thread : threads_)
    {
        if(tid == thread->getTID())
        {
            return thread;
        }
    }
    return nullptr;
}
