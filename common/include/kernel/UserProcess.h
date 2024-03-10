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
    int32 tid_counter_;

    // added by Minh
  public:
    void createUserThread(void* func, void* para);



    /* 
    Could be added from Thread:
      - Terminal* my_terminal_;
      - Thread* next_thread_in_lock_waiters_list_;
      - Lock* lock_waiting_on_;
      - Lock* holding_lock_list_;

    Could be added (from Minh):
      - Tid counter for unique thread id (Note that Thead.h already has tid_)
    */
};

