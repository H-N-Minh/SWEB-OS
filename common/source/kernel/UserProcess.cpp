#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "UserProcess.h"

#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "types.h"

//int64 UserProcess::tid_counter_ = 1; //numthread in Tai case
int64 UserProcess::pid_counter_ = 1;
//todo: use ArchThreads::atomic_add

//--------------------------MINH PART----------------------------
//UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
//        : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info),
//          filename_(filename), terminal_number_(terminal_number), loader_(0), pid_(pid_counter_++), 
            // thread_counter_lock_("thread_counter_lock_"),
            // threads_lock_("thread_lock_"), value_ptr_by_id_lock_("value_ptr_by_id_lock_")
        

// UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
//             : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), num_thread_(1),
//               filename_(filename), terminal_number_(terminal_number)
            // thread_counter_lock_("thread_counter_lock_"),
            // threads_lock_("thread_lock_"), value_ptr_by_id_lock_("value_ptr_by_id_lock_")
// {
//     ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

//     if (fd_ >= 0)
//         loader_ = new Loader(fd_);

//     if (!loader_ || !loader_->loadExecutableAndInitProcess())
//     {
//         debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
//         //kill();           //!
//         //to_be_destroyed_ = true;     //123
//         return;
//     }

//     threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this,
//                                       num_thread_, NULL, NULL, NULL));
//     num_thread_++;
//     debug(TAI_THREAD, "num thread %zu\n", num_thread_);
//     debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

    //---------------------MINH part------------------
//    if (!loader_ || !loader_->loadExecutableAndInitProcess())
//  {
//    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
//    //kill();           // This belong to Thread, not sure what to do here
//    return;
//  }
//
//  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this,
//                                    tid_counter_, NULL, NULL, NULL));
//  ArchThreads::atomic_add(tid_counter_, 1);
//  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
//}

//--------------------------MINH PART----------------------------
//Note: tid_counter is num_thread in Tais case

//UserProcess::UserProcess(const UserProcess& other)
//  : fd_(VfsSyscall::open(other.filename_, O_RDONLY)), working_dir_(new FileSystemInfo(*other.working_dir_)),
//    filename_(other.filename_), terminal_number_(other.terminal_number_), loader_(0), pid_(pid_counter_)
//{
//  debug(USERPROCESS, "Copy-ctor: start copying from process (%u) \n", other.pid_);
//  ArchThreads::atomic_add(pid_counter_, 1);
//  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
//
//  assert(fd_ >= 0  && "Error: File descriptor doesnt exist, Loading failed in UserProcess copy-ctor\n");
//  debug(USERPROCESS, "Copy-ctor: Calling Archmemory copy-ctor for new Loader\n");
//  loader_ = new Loader(fd_, other.loader_->arch_memory_);
//
//  debug(USERPROCESS, "Copy-ctor: Done loading with new ArchMemory, now calling copy-ctor for Thread\n");
//  UserThread* new_thread = new UserThread(*(UserThread*) currentThread, this, tid_counter_, terminal_number_, loader_);
//  threads_.push_back(new_thread);
//
//  debug(USERPROCESS, "Copy-ctor: Done copying Thread, adding new thread id (%zu) to the Scheduler", new_thread->getTID());
//  Scheduler::instance()->addNewThread(new_thread);
//  ArchThreads::atomic_add(tid_counter_, 1);
//}

//from Steffi
//size_t UserProcess::pid_counter_ = 0;

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number) : fd_(VfsSyscall::open(filename, O_RDONLY)), 
        working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number), thread_counter_lock_("thread_counter_lock_"),
        threads_lock_("thread_lock_"), value_ptr_by_id_lock_("value_ptr_by_id_lock_")
        
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           //TODO
    return;
  }
  thread_counter_++;            //should be fine without locking since we are still singlethreaded
  UserThread* new_thread = new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0, thread_counter_, false);
  threads_.push_back(new_thread); 
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  pid_counter_++;         //Todo:locking
  pid_ = pid_counter_;
}



UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = 0;

    ProcessRegistry::instance()->processExit();
}

UserThread* UserProcess::getUserThread(size_t tid)
{
  for (size_t i = 0; i < threads_.size(); i++)
  {
    if (threads_[i]->getTID() == tid)
      return threads_[i];
  }
  return 0;
}

//from steffi
int UserProcess::create_thread(size_t* thread, void *(*start_routine)(void*), void *(*wrapper)(), void* arg)
{
  debug(USERPROCESS, "Unused %p, %p, %p, %p", thread, start_routine, wrapper, arg);
  thread_counter_lock_.acquire();
  thread_counter_++;
  threads_lock_.acquire();  
  UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_, this, start_routine, wrapper, arg, thread_counter_, false);
  thread_counter_lock_.release();
  if(new_thread)
  {
    threads_.push_back(new_thread);
    Scheduler::instance()->addNewThread(new_thread);
    *thread = new_thread->getTID();
    threads_lock_.release();  
    return 0;
  }
  else
  {
    threads_lock_.release();  
    return -1;
  }
}


UserThread* UserProcess::get_thread_from_threadlist(size_t id)
{
  for (auto& thread : threads_)
  {
    if(id == thread->getTID())
    {
      return thread;
    } 
  }
  return NULL;
}

bool UserProcess::check_if_thread_in_threadList(UserThread* test_thread)
{
  for (auto& thread : threads_)
  {
    if(test_thread == thread)
    {
      return true;
    } 
  }
  return false;
}





void UserProcess::createThread(void* func, void* para, size_t* tid, void* pcreate_helper)
{
    debug(USERPROCESS, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
    UserThread* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_,
                                            ((UserThread*) currentThread)->process_, num_thread_, func, para, pcreate_helper);
    threads_.push_back(new_thread);
    *((unsigned long*) tid) = (unsigned long) num_thread_;
    debug(TAI_THREAD, "--------------------- TID %zu \n", *tid);
    num_thread_++;

    //----------FROM MINH Fork----------------
    //ArchThreads::atomic_add(tid_counter_, 1);

    debug(USERPROCESS, "UserProcess::createUserThread: Adding new thread to scheduler\n");
    Scheduler::instance()->addNewThread(new_thread);
}
