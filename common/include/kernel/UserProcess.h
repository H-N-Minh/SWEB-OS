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
    

    ustl::vector<UserThread*> threads_;
    ustl::map<size_t, void*> value_ptr_by_id_;

    int32 fd_;  //TODO: lock ?                   
    Loader* loader_;  //TODO: lock ? 
    FileSystemInfo* working_dir_;  //TODO: lock ? 
    uint32 terminal_number_;  //TODO: lock ? 
    ustl::string filename_;  //TODO: lock ? 

    size_t thread_counter_{0};   //gets currently also increased if thread_creation was not succesfull


    Mutex thread_counter_lock_;    //locking order 1
    Mutex threads_lock_;           //2
};

