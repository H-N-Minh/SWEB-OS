#pragma once
#include "UserThread.h"
#include "uvector.h"
#include "umap.h"
#include "Mutex.h"

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

    int create_thread(size_t* thread, void *(*start_routine)(void*), void *(*wrapper)(), void* arg);
    UserThread* get_thread_from_threadlist(size_t id);
    bool check_if_thread_in_threadList(UserThread* test_thread);
    

    ustl::vector<UserThread*> threads_;
    ustl::map<size_t, void*> value_ptr_by_id_;


    int32 fd_;  //TODO: lock ?                   
    Loader* loader_;  //TODO: lock ? 
    FileSystemInfo* working_dir_;  //TODO: lock ? 
    uint32 terminal_number_;  //TODO: lock ? 
    ustl::string filename_;  //TODO: lock ?    

    size_t thread_counter_{0};   //TODO: not really counts the thread


    Mutex thread_counter_lock_;    //1         //Todo: check if the locking order is actually followed
    Mutex threads_lock_;           //2
    Mutex value_ptr_by_id_lock_;   //3
  


    Loader* execv_loader_{NULL}; //needs a loock
    int32 execv_fd_{NULL};  //TODO: lock ?     
    size_t execv_ppn_args_{NULL};  //TODO: lock ? 
    size_t exec_argc_{0};  //TODO: lock ? 

    static size_t pid_counter_; //lock lock
    size_t pid_;                //Todo: lock
    size_t parent_pid_{0};

   
};

