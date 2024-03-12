#pragma once
#include "UserThread.h"
#include "uvector.h"
#include "umap.h"

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
    bool to_be_destroyed_ = false;

  private:
    int32 fd_;
    Loader* loader_;
    FileSystemInfo* working_dir_;

    /////////////////////////////////////////////// added by Minh//////////////////////////////////////////
    int32 tid_counter_;
    ustl::string filename_;
    uint32 terminal_number_;
  public:
    ustl::map <uint32, UserThread*> p_join_sleep_map_;   // map of the thread that is sleeping/waiting for the return value
    //TODO: currently this map only let 1 thread wait for 1 tid, can change key and value place to solve this
    ustl::map <uint32, void*> result_storage_;   // when a thread finishes, it stores its return value here

    void createUserThread(void* func, void* para, void* tid, void* pcreate_helper);
    void storeThreadRetval(uint32 tid, void* retval);   
    void retrieveThreadRetval(uint32 target_tid, UserThread* waiter_thread, void** retval);

    /* 
    Could be added from Thread:
      - Terminal* my_terminal_;
      - Thread* next_thread_in_lock_waiters_list_;
      - Lock* lock_waiting_on_;
      - Lock* holding_lock_list_;
    */
};

