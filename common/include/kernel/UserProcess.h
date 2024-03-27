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

        /**
         * Returns thread by id, if it is in threads_, else return zero
         * @param tid thread_id
         *
         */
        UserThread* getUserThread(size_t tid);
        bool isThreadInVector(UserThread* test_thread);

        /**
         * Checks if retval of thread with tid is in thread_retval map and removes it.
         * If value_ptr != NULL it stores the return value into the given address.
         * Returns 0 on success and -1 on failure.
         * @param tid thread_id
         * @param value_ptr address to location, where return value should get stored
         */
        int removeRetvalFromMapAndSetReval(size_t tid, void**value_ptr);

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg);
        int joinThread(size_t thread_id, void**value_ptr);

        int execvProcess(const char *path, char *const argv[]);

        static int64 pid_counter_;
        int32 pid_;

        int32 fd_;                                 //Todo: figure out, what needs to be locked here
        FileSystemInfo* working_dir_;
        ustl::string filename_;
        uint32 terminal_number_;
        Loader* loader_;

        //Threads
        static int64 tid_counter_;
        ustl::vector<UserThread*> threads_;
        Mutex threads_lock_;                            //Locking order: x (TODO)

        //Return value map (locked by threads lock)
        ustl::map<size_t, void*> thread_retval_map_;

        //Exec
        Mutex execv_lock_;                              //Locking order: x
        Loader* execv_loader_{NULL};
        int32 execv_fd_{NULL};  
        size_t execv_ppn_args_{NULL};                   
        size_t exec_argc_{0};    
};

