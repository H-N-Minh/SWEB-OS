#pragma once

#include "Thread.h"
#include "uvector.h"

#include "UserThread.h"

class UserProcess
//        : public Thread
{
  public:
    /**
     * Constructor
     * @param minixfs_filename filename of the file in minixfs to execute
     * @param fs_info filesysteminfo-object to be used
     * @param terminal_number the terminal to run in (default 0)
     *
     */
    UserProcess(ustl::string minixfs_filename, FileSystemInfo *fs_info, uint32 terminal_number = 0);

    virtual ~UserProcess();

    virtual void Run(); // not used

    Thread* createThread(ustl::string thread_name);
    void terminateThread(Thread* thread);
    ustl::vector<Thread*> getThread();

  private:
    int32 fd_;
    ustl::vector<Thread*> threads_;
    Loader* loader_;
    FileSystemInfo* working_dir_;
    ustl::string filename_;
    uint32 terminal_number_;
};

