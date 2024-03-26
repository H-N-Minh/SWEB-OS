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
        UserProcess(const UserProcess& other);

        UserThread* getUserThread(size_t tid);
        bool isThreadInVector(UserThread* test_thread);
        int removeRetvalFromMapAndSetReval(size_t tid, void**value_ptr);

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg);
        int joinThread(size_t thread_id, void**value_ptr);

        int execvProcess(const char *path, char *const argv[]);


        ustl::vector<UserThread*> threads_;
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

        Mutex execv_lock_;   //todo locking
        Loader* execv_loader_{NULL};
        int32 execv_fd_{NULL};  
        size_t execv_ppn_args_{NULL};  
        size_t exec_argc_{0};    
};

