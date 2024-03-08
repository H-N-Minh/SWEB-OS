#pragma once
#include "UserThread.h"
#include "uvector.h"

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

    ustl::vector<UserThread*> threads_;          //!!
    bool to_be_destroyed_ = false;
  private:
    int32 fd_;
    Loader* loader_;
    FileSystemInfo* working_dir_;
};

