#pragma once

#include <types.h>

#include "Thread.h"

class Syscall
{
  public:
    static size_t syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

    static void exit(size_t exit_code, bool from_exec = false);
    static void outline(size_t port, pointer text);

    static size_t write(size_t fd, pointer buffer, size_t size);
    static size_t read(size_t fd, pointer buffer, size_t count);
    static size_t close(size_t fd);
    static size_t open(size_t path, size_t flags);
    static void pseudols(const char *pathname, char *buffer, size_t size);

    static size_t createprocess(size_t path, size_t sleep);
    static void trace();

    static uint32 get_thread_count();

    static l_off_t lseek(size_t fd, l_off_t offset, uint8 whence);


    static uint32 pipe(int file_descriptor_array[2]);


    static int pthreadCreate(size_t* thread, unsigned int* attr, void* start_routine, void* arg, void* wrapper_address) ;
    static void pthreadExit(void* value_ptr); 
    static int pthreadJoin(size_t thread_id, void**value_ptr); 

    /**
     * If thread is active, set join_state to Detached, else remove the retval of the dead thread from retval_map
     * @param is_threads_vector_locked if the threads_ vector is already locked, set this to true. Default is false
     * @return 0 on success, -1 if the given thread is already Detached or the given thread doesnt have a retval in retval_map
    */
    static int pthreadDetach(size_t thread,  bool is_threads_vector_locked = false);

    /**
      * @param is_threads_vector_locked if true: threads_ vector already locked (locked in Exit()).
      * @return -1 if thread doesnt exist in vector. else return 0: set the flag wants_to_be_canceled_ to true.
    */
    static int pthreadCancel(size_t thread_id, bool is_threads_vector_locked = false);

    static unsigned int sleep(unsigned int seconds);

    static unsigned int clock(void);

    static int execv(const char *path, char *const argv[]);

    /**
     * check if the given pointer is user space and not null
    */
    static bool check_parameter(size_t ptr, bool allowed_to_be_null = false);

    static void send_cancelation_notification(bool to_late = false);

    static int pthread_setcancelstate(int state, int *oldstate);
    static int pthread_setcanceltype(int type, int *oldtype);

    static uint32 forkProcess();

    static uint64_t get_current_timestamp_64_bit();

    // minhsbrk2
    // static size_t sbrkMemory(size_t size_ptr, size_t return_ptr);
    // static size_t brkMemory(size_t new_brk_addr);
};

