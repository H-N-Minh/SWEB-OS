#pragma once
#include "UserThread.h"
#include "uvector.h"
#include "umap.h"
#include "Mutex.h"
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

        // COPY CONSTRUCTOR
        UserProcess(const UserProcess& other);

        ustl::vector<UserThread*> threads_;

        UserThread* getUserThread(size_t tid);

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg);

        bool isThreadInVector(UserThread* test_thread);

        [[maybe_unused]] size_t openFile(const ustl::string& path, uint32_t mode);


        int32 fd_;
        FileSystemInfo* working_dir_;
        ustl::string filename_;
        uint32 terminal_number_;
        Loader* loader_;
        int32 pid_;
        static int64 tid_counter_;
        static int64 pid_counter_;

        ustl::map<size_t, void*> thread_retval_map_;        // stores return value after thread finished

        Mutex threads_lock_;           //2         //Todo: check if the locking order is actually followed
        Mutex thread_retval_map_lock_;   //3

        Loader* execv_loader_{NULL}; //needs a loock
        int32 execv_fd_{NULL};  //TODO: lock ?     
        size_t execv_ppn_args_{NULL};  //TODO: lock ? 
        size_t exec_argc_{0};  //TODO: lock ?
};

