#pragma once
#include "UserThread.h"
#include "uvector.h"
#include "umap.h"
#include "Mutex.h"
#include "Condition.h"
#include "types.h"

#include "UserSpaceMemoryManager.h"
#include "LocalFileDescriptorTable.h"


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

        int createThread(size_t* thread, void* start_routine, void* wrapper, void* arg);

        bool isThreadInVector(UserThread* test_thread);


        
         //bool isThreadInVector(UserThread* test_thread);


        /**
         * Checks if retval of thread with tid is in thread_retval map and removes it.
         * If value_ptr != NULL it stores the return value into the given address.
         * Returns 0 on success and -1 on failure.
         * @param tid thread_id
         * @param value_ptr address to location, where return value should get stored
         */
        int removeRetvalFromMapAndSetReval(size_t tid, void**value_ptr);



        int execvProcess(const char *path, char *const argv[]);
        void exitProcess(size_t exit_code);
        void cancelAllOtherThreads();


        bool check_parameters_for_exec(char *const argv[], int& argc, int& array_offset);
        void waitForThreadsToDie();


        void unmapThreadStack(ArchMemory* arch_memory, size_t top_stack);

        static int64 pid_counter_;
        int32 pid_;

        int32 fd_;                                 //TODOs: figure out, what needs to be locked and locking order
        FileSystemInfo* working_dir_;
        ustl::string filename_;
        uint32 terminal_number_;
        Loader* loader_;
        UserSpaceMemoryManager* user_mem_manager_;

        //Threads
        static int64 tid_counter_;
        ustl::vector<UserThread*> threads_;
        Mutex threads_lock_;                            //Locking order: x

        //Return value map (locked by threads lock)
        ustl::map<size_t, void*> thread_retval_map_;

        
        bool one_thread_left_{false};
        Mutex one_thread_left_lock_;                                //Locking order: x
        Condition one_thread_left_condition_;

        uint64_t clock_{0};
        uint64_t tsc_start_scheduling_{0};

        LocalFileDescriptorTable localFileDescriptorTable;

        ustl::string str() const;
};

