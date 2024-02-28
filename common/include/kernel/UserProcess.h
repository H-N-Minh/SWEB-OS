#pragma once

#include "UserThread.h"

class UserProcess
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

    ustl::vector<UserThread*> threads;

  private:
    int32 fd_;
    Loader* loader_;
};

