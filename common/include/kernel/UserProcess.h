#pragma once
#include "UserThread.h"
#include "uvector.h"
#include "types.h"

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

    ustl::vector<UserThread*> threads_;

  private:
    int32 fd_;
    FileSystemInfo* working_dir_;

    ////////////////////////////////////////////////////////////////////////////////////////
    static int64 tid_counter_;
    static int64 pid_counter_;
    ustl::string filename_;
    uint32 terminal_number_;

  public:
    Loader* loader_;

    int32 pid_;
    
    UserProcess(const UserProcess& other);

    void createUserThread(void* func, void* para, void* tid, void* pcreate_helper);

    UserThread* getUserThread(size_t tid);

    /* 
    Could be added from Thread:
      - Terminal* my_terminal_;
      - Thread* next_thread_in_lock_waiters_list_;
      - Lock* lock_waiting_on_;
      - Lock* holding_lock_list_;
    */
};

