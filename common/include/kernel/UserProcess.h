#pragma once

#include "FileSystemInfo.h"
#include "Loader.h"
#include "Terminal.h"
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
    UserProcess(ustl::string filename, FileSystemInfo* fs_info, uint32 terminal_number = 0);
    ~UserProcess();

    void Run(); // either no use or needs to be implemented

    Terminal* getTerminal();
    void setTerminal(Terminal* my_term);

    FileSystemInfo* getWorkingDirInfo();
    void setWorkingDirInfo(FileSystemInfo* working_dir);

    ustl::vector<UserThread*> threads;

  private:
    int32 fd_;
    Loader* loader_;
    FileSystemInfo* working_dir_;
    Terminal* my_terminal_;

    ustl::string filename_;
    uint32 terminal_number_;

    void createInitialThread();
};

