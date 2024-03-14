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

        ustl::vector<UserThread*> threads_;
        bool to_be_destroyed_ = false;

        void createThread(void *(*start_routine)(void*), void* para);

        UserThread* getUserThread(size_t tid);
    private:
        int32 fd_;
        Loader* loader_;
        FileSystemInfo* working_dir_;

        size_t num_thread_;
        ustl::string filename_;
        uint32 terminal_number_;
};

