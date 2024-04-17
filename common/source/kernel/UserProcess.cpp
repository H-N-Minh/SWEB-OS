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
#include "Syscall.h"

#define POINTER_SIZE 8

int64 UserProcess::tid_counter_ = 1;
int64 UserProcess::pid_counter_ = 1;


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
  : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number),
    threads_lock_("thread_lock_"),  execv_lock_(" execv_lock_"), one_thread_left_lock_("one_thread_left_lock_"),
    one_thread_left_condition_(&one_thread_left_lock_, "one_thread_left_condition_")
{
  ProcessRegistry::instance()->processStart();
  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    delete loader_;
    loader_ = 0;
    VfsSyscall::close(fd_);
    delete working_dir_;
    working_dir_ = 0;
    return;
  }
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0, false));
  debug(USERPROCESS, "ctor: Done creating Thread\n");
}


// COPY CONSTRUCTOR
UserProcess::UserProcess(const UserProcess& other)
  : fd_(VfsSyscall::open(other.filename_, O_RDONLY)), working_dir_(new FileSystemInfo(*other.working_dir_)), filename_(other.filename_), 
    terminal_number_(other.terminal_number_), threads_lock_("thread_lock_"), execv_lock_(" execv_lock_"),
    one_thread_left_lock_("one_thread_left_lock_"), one_thread_left_condition_(&one_thread_left_lock_, "one_thread_left_condition_")
{
  debug(FORK, "Copy-ctor UserProcess: start copying from process (pid:%u) \n", other.pid_);
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  pid_ = ArchThreads::atomic_add(pid_counter_, 1);

  assert(fd_ >= 0  && "Error: File descriptor doesnt exist, Loading failed in UserProcess copy-ctor\n");
  debug(USERPROCESS, "Copy-ctor: Calling Archmemory copy-ctor for new Loader\n");
  other.loader_->arch_memory_.lock_.acquire();
  loader_ = new Loader(*other.loader_, fd_);
  other.loader_->arch_memory_.lock_.release();
  if (!loader_){assert(0 && "No loader in fork");}

  UserThread* child_thread = new UserThread(*(UserThread*) currentThread, this);
  threads_.push_back(child_thread);

  debug(USERPROCESS, "Copy-ctor: Done copying Thread, adding new thread id (%zu) to the Scheduler", child_thread->getTID());
  Scheduler::instance()->addNewThread(child_thread);
}


UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  debug(USERPROCESS, "Delete loader %p from process %d.\n", loader_, pid_);
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = nullptr;


  /////
  //local_fd_table_.closeAllFileDescriptors();
  /////

  ProcessRegistry::instance()->processExit();
}


UserThread* UserProcess::getUserThread(size_t tid)
{
  assert(threads_lock_.heldBy() == currentThread && "getUserThread used without holding threads_lock");
  for (size_t i = 0; i < threads_.size(); i++)
  {
    if (threads_[i]->getTID() == tid)
      return threads_[i];
  }
  return 0;
}


int UserProcess::removeRetvalFromMapAndSetReval(size_t tid, void**value_ptr)
{
  assert(threads_lock_.heldBy() == currentThread && "getUserThread used without holding threads_lock");
  ustl::map<size_t, void*>::iterator iterator = thread_retval_map_.find(tid);                                                   
  if(iterator != thread_retval_map_.end())
  {
    if(value_ptr != NULL)
    {
      *value_ptr = thread_retval_map_[tid];
    }
    thread_retval_map_.erase(iterator);
    return 0;
  }
  else
  {
    return -1;
  }
}


bool UserProcess::isThreadInVector(UserThread* test_thread)
{
  //TODO: check if lock is held
  for (auto& thread : threads_)
  {
    if(test_thread == thread)
    {
      return true;
    } 
  }
  return false;
}


int UserProcess::execvProcess(const char *path, char *const argv[])
{
  if(!Syscall::check_parameter((size_t)path, false) || !Syscall::check_parameter((size_t)argv, false))
  {
    return -1;
  }
  UserThread& currentUserThread = *((UserThread*)currentThread);
  
  int space_left = 4000;   //page size (more or less)
  size_t array_offset = 0;

  bool finished = false;

  //go through all arguments, check if the are valid and if there is enough space
  int argc = 0;
  while(!finished)
  {
    if(!Syscall::check_parameter((size_t)argv[argc], true))
    { 
      debug(USERPROCESS, "Execv: parameters not in userspace\n");
      return -1;
    }

    if(argv[argc] == NULL)
    {
      space_left-= POINTER_SIZE;
      finished = true;
    }
    else
    {
      space_left-= (strlen(argv[argc]) + 1 + POINTER_SIZE);
      array_offset+= strlen(argv[argc]) + 1;
      argc++;
    }
    if(space_left < 0)
    {
      debug(USERPROCESS, "Execv: no space left\n");
      return -1;
    } 
  }


  //open the filedescriptor of the new program
  execv_lock_.acquire();
  //
  currentUserThread.cancel_state_type_lock_.acquire();
  if(((UserThread*)currentThread)->cancel_type_ == PTHREAD_CANCEL_EXIT)
  {
    currentUserThread.cancel_state_type_lock_.release();
    execv_lock_.release();
    return -1;
  }
  currentUserThread.cancel_state_type_lock_.release();


  execv_fd_ = VfsSyscall::open(path, O_RDONLY);
  if(execv_fd_ < 0)
  {
    execv_lock_.release();
    return -1;
  }

  //create loader for the new binary                                 
  execv_loader_ = new Loader(execv_fd_);
  if (!execv_loader_ || !execv_loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", path);
    delete execv_loader_;
    execv_loader_ = 0;
    VfsSyscall::close(execv_fd_);
    execv_lock_.release();
    return -1;
  }

  //cancel all other threads
  threads_lock_.acquire();
  cancelAllOtherThreads();
  currentUserThread.send_kill_notification(); //todos: in case a thread wants to join me but what happens if later still wants to join
  execv_lock_.release();
  

  


  
  if(threads_.size() > 1)
  {
    one_thread_left_ = false;
  }
  else
  {
    one_thread_left_ = true;
  }
  threads_lock_.release();
  //wait for the other threads to die
  one_thread_left_lock_.acquire();
  while(!one_thread_left_)
  {
    one_thread_left_condition_.wait();
  }
  one_thread_left_lock_.release();

  thread_retval_map_.clear();


  


  //allocate a free physical page and get the virtual address of the identity mapping
  execv_ppn_args_ = PageManager::instance()->allocPPN();
  size_t virtual_address =  ArchMemory::getIdentAddressOfPPN(execv_ppn_args_);

  exec_array_offset_ = array_offset;
  size_t offset = 0;
  size_t offset1 = USER_BREAK - PAGE_SIZE;
  
  for(int i = 0; i < argc; i++)
  {
    //write the arguments one by one to the new phsical page via identity mapping
    memcpy((char*)virtual_address + offset, argv[i], strlen(argv[i])+1);

    //store the offset of each argument in the page, at the end of all arguments
    memcpy((void*)(virtual_address + exec_array_offset_ + i * POINTER_SIZE), &offset1, POINTER_SIZE);
    offset += strlen(argv[i]) + 1;
    offset1 += strlen(argv[i]) + 1;
  }
  if(argc > 0)
  {
    //storing the pointer to the virtual address of the single elements in the array
    memset((void*)(virtual_address + exec_array_offset_ + argc * POINTER_SIZE), NULL, POINTER_SIZE);
  }
  
  exec_argc_ = argc;



  //execv_lock_.release();
  ustl::vector<UserThread*>::iterator exiting_thread = ustl::find(threads_.begin(), threads_.end(), currentThread);
  threads_.erase(exiting_thread);
  ((UserThread*)currentThread)->last_thread_before_exec_ = true;
  ((UserThread*)currentThread)->kill();

  
  assert(0 && "Sucessful exec should not return");

}


void UserProcess::exitProcess(size_t exit_code)
{
  UserThread& currentUserThread = *((UserThread*)currentThread);

  debug(USERPROCESS, "UserProcess::exitProcess: Thread (%zu) called exit_code: %zd.\n", currentUserThread.getTID(), exit_code);

  threads_lock_.acquire();
  cancelAllOtherThreads();
  threads_lock_.release();

  debug(USERPROCESS, "UserProcess::exitProcess: Last Thread %zu calls pthread exit. \n", currentUserThread.getTID());
  currentUserThread.exitThread((void*)exit_code);
  assert(false && "This should never happen");

}

void UserProcess::cancelAllOtherThreads()
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  assert(threads_lock_.heldBy() == currentThread);

  for (auto& thread : threads_)
  {
    if(thread != &currentUserThread)
    {
      size_t thread_id = thread->getTID();
      thread->cancel_state_type_lock_.acquire();
      thread->cancel_type_ = PTHREAD_CANCEL_EXIT;  
      thread->cancel_state_type_lock_.release();
      debug(USERPROCESS, "UserProcess::exitProcess: Thread %zu gets canceled. \n",thread_id);
      currentUserThread.detachThread(thread_id);
      assert(currentUserThread.cancelThread(thread_id) == 0 && "pthreadCancel failed in exit.\n");
      debug(USERPROCESS, "UserProcess::exitProcess: Thread %zu was canceled sucessfully. \n",thread_id);
    } 
  }


}