#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"

#include "ArchInterrupts.h"


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) 
    : fd_(VfsSyscall::open(filename, O_RDONLY)), loader_(0), working_dir_(fs_info), tid_counter_(1), 
      filename_(filename), terminal_number_(terminal_number)
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           // This belong to Thread, not sure what to do here
    return;
  }
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 
                                    tid_counter_, NULL, NULL, NULL));
  tid_counter_++;
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  ProcessRegistry::instance()->processExit();
}

void UserProcess::createUserThread(void* func, void* para, void* tid, void* pcreate_helper)
{
  debug(USERPROCESS, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, 
                                          ((UserThread*) currentThread)->process_, tid_counter_, func, para, pcreate_helper);
  threads_.push_back(new_thread);
  *((unsigned long*) tid) = (unsigned long) tid_counter_;
  tid_counter_++;

  debug(USERPROCESS, "UserProcess::createUserThread: Adding new thread to scheduler\n");
  // Scheduler::instance()->printThreadList();
  Scheduler::instance()->addNewThread(new_thread);
  // Scheduler::instance()->printThreadList();
}

// TODO: this is not thread-safe when pt_join thread tries to gets the result at the same time as this thread
// try to write the result. 
// Can be solved by locking both p_join_sleep_map_ and result_storage_
/**
 * Store the return value of a thread, then wake up all threads that are waiting for this thread to finish
*/
void UserProcess::storeThreadRetval(uint32 tid, void* retval)
{
  debug(USERPROCESS, "UserProcess::storeThreadRetval: storing for thread %d the return value %zu\n", tid, (size_t) retval);

  // tid_counter_++;
  // result_storage_[tid] = retval;
  // if (p_join_sleep_map_.find(tid) != p_join_sleep_map_.end())
  // {
  //   debug(USERPROCESS, "UserProcess::storeThreadRetval: waking up sleeping thread %d\n", tid);
  //   Scheduler::instance()->wake(p_join_sleep_map_[tid]);
  //   p_join_sleep_map_.erase(tid);
  // }
}

// get return value of a thread, if the retval is not ready then put it to sleep
void UserProcess::retrieveThreadRetval(uint32 target_tid, UserThread* waiter_thread, void** retval)
{
  debug(USERPROCESS, "UserProcess::retrieveThreadRetval: thread (%d) is not finished yet, putting thread (%zu) to sleep\n", 
                        target_tid, waiter_thread->getTID());
  (void) retval;
  // // if the result is not ready yet
  // if (result_storage_.find(target_tid) == result_storage_.end())
  // {
  //   debug(USERPROCESS, "UserProcess::retrieveThreadRetval: thread (%d) is not finished yet, putting thread (%zu) to sleep\n", 
  //                       target_tid, waiter_thread->getTID());

  //   p_join_sleep_map_[target_tid] = waiter_thread;
  //   Scheduler::instance()->sleep();
  // }

  // // When this thread got waken up, it continues here
  // assert(result_storage_.find(target_tid) != result_storage_.end() && "Waiter Thread is woken up but the retval is still not ready\n");
  // debug(USERPROCESS, "UserProcess::retrieveThreadRetval: found the return value for thread %d\n", target_tid);

  // *retval = result_storage_[target_tid];
  // result_storage_.erase(target_tid);
}

