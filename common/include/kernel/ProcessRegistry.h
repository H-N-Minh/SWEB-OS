#pragma once

#include "UserProcess.h"
#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

class ProcessRegistry : public Thread
{
  public:
    /**
     * Constructor
     * @param root_fs_info the FileSystemInfo
     * @param progs a string-array of the userprograms which should be executed
     */
    ProcessRegistry ( FileSystemInfo *root_fs_info, char const *progs[] );
    ~ProcessRegistry();

    /**
     * Mounts the Minix-Partition with user-programs and creates processes
     */
    virtual void Run();

    /**
     * Tells us that a userprocess is being destroyed
     */
    void processExit();

    /**
     * Tells us that a userprocess is being created due to a fork or something similar
     */
    void processStart();

    /**
     * Tells us how many processes are running
     */
    size_t processCount();

    static ProcessRegistry* instance();
    void createProcess(const char* path);

    Condition process_exit_status_map_condition_;

    Mutex process_exit_status_map_lock_;
    ustl::map<size_t, size_t> process_exit_status_map_;

    ustl::vector<UserProcess*> processes_;
    Mutex processes_lock_;


  private:

    char const **progs_;
    uint32 progs_running_;
    Mutex counter_lock_;
    Condition all_processes_killed_;
    static ProcessRegistry* instance_;
};

