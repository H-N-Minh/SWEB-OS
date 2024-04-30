#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "types.h"
#include "Syscall.h"

#include "sostream.h"


#include "UserThread.h"


#define POINTER_SIZE 8

int64 UserProcess::tid_counter_ = 2;
int64 UserProcess::pid_counter_ = 1;


UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)

  : fd_(VfsSyscall::open(filename, O_RDONLY)), working_dir_(fs_info), filename_(filename), terminal_number_(terminal_number),

    threads_lock_("thread_lock_"), one_thread_left_lock_("one_thread_left_lock_"),
    one_thread_left_condition_(&one_thread_left_lock_, "one_thread_left_condition_"), localFileDescriptorTable()
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

  user_mem_manager_ = new UserSpaceMemoryManager(loader_);

  pid_ = ArchThreads::atomic_add(pid_counter_, 1);
  debug(WAIT_PID, "-----------------------pid_ in constructor %d \n", pid_);


  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 0, 0, 0));
  debug(USERPROCESS, "ctor: Done creating Thread\n");

}

// COPY CONSTRUCTOR
UserProcess::UserProcess(const UserProcess& other)
    : fd_(VfsSyscall::open(other.filename_, O_RDONLY)), working_dir_(new FileSystemInfo(*other.working_dir_)), filename_(other.filename_),

      terminal_number_(other.terminal_number_), threads_lock_("thread_lock_"),
      one_thread_left_lock_("one_thread_left_lock_"), one_thread_left_condition_(&one_thread_left_lock_, "one_thread_left_condition_"), localFileDescriptorTable()
{
  debug(FORK, "Copy-ctor UserProcess: start copying from process (pid:%u) \n", other.pid_);
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process
  pid_ = ArchThreads::atomic_add(pid_counter_, 1);
  debug(WAIT_PID, "-----------------------pid_ in cpy constructor %d \n", pid_);

  assert(fd_ >= 0  && "Error: File descriptor doesnt exist, Loading failed in UserProcess copy-ctor\n");
  debug(USERPROCESS, "Copy-ctor: Calling Archmemory copy-ctor for new Loader\n");
  other.loader_->arch_memory_.lock_.acquire();
  loader_ = new Loader(*other.loader_, fd_);
  other.loader_->arch_memory_.lock_.release();
  if (!loader_){assert(0 && "No loader in fork");}

  user_mem_manager_ = new UserSpaceMemoryManager(loader_);

  UserThread* child_thread = new UserThread(*(UserThread*) currentThread, this);
  threads_.push_back(child_thread);

  for(auto &fd : other.localFileDescriptorTable.getLocalFileDescriptors())
  {
    auto* newLFD = new LocalFileDescriptor(*fd);
    debug(FILEDESCRIPTOR, "UserProcess::ctor: Global FD = %u; RefCount = %d\n", newLFD->getGlobalFileDescriptor()->getFd(), newLFD->getGlobalFileDescriptor()->getRefCount());

    int permissions = newLFD->getMode();
    debug(FILEDESCRIPTOR, "Copy-ctor: Copied Local FD: %zu Permissions: %d\n", newLFD->getLocalFD(), permissions);
    newLFD->getGlobalFileDescriptor()->incrementRefCount();
    debug(Fabi, "UserProcess:: Global FD = %u; Local FD = %zu; RefCount = %d\n; Process ID = %u\n; Filename = %s", newLFD->getGlobalFileDescriptor()->getFd(), newLFD->getLocalFD(), newLFD->getGlobalFileDescriptor()->getRefCount(), pid_, filename_.c_str());

    this->localFileDescriptorTable.addLocalFileDescriptor(newLFD);
  }

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

  localFileDescriptorTable.closeAllFileDescriptors();
  delete working_dir_;
  working_dir_ = nullptr;

  delete user_mem_manager_;
  user_mem_manager_ = nullptr;


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


int UserProcess::removeRetvalFromMapAndSetReval(size_t tid, void*& return_value)
{
  assert(threads_lock_.heldBy() == currentThread && "getUserThread used without holding threads_lock");
  ustl::map<size_t, void*>::iterator iterator = thread_retval_map_.find(tid);                                                   
  if(iterator != thread_retval_map_.end())
  {
    return_value = thread_retval_map_[tid];
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

void UserProcess::unmapThreadStack(ArchMemory* arch_memory, size_t top_stack)
{
  debug(SYSCALL, "pthreadExit: Unmapping thread's stack\n");
  assert(top_stack && "Error: top_stack is NULL in unmapThreadStack\n");
  assert(arch_memory && "Error: arch_memory is NULL in unmapThreadStack\n");

  uint64 top_vpn = (top_stack + sizeof(size_t)) / PAGE_SIZE - 1;
  for (size_t i = 0; i < MAX_STACK_AMOUNT; i++)
  {
    if (arch_memory->checkAddressValid(top_stack))
    {
      size_t* guard1 = (size_t*) top_stack;
      size_t* guard2 = (size_t*) (top_stack - sizeof(size_t) * (META_SIZE - 1) );
      *guard1 = 0;
      *guard2 = 0;
      arch_memory->lock_.acquire();
      arch_memory->unmapPage(top_vpn);
      arch_memory->lock_.release();
      top_vpn--;
      top_stack -= PAGE_SIZE;
    }
    else
    {
      break;
    }
  }

  debug(SYSCALL, "pthreadExit: Unmapping thread's stack done\n");
}

void UserProcess::write_to_page(size_t ppn, size_t next_page, size_t offset, char* string, int len_string)
{
  size_t virtual_address = ArchMemory::getIdentAddressOfPPN(ppn);
  size_t virtual_address_2;
  if(next_page)
  {
    virtual_address_2 = ArchMemory::getIdentAddressOfPPN(next_page);
  }

  //everythings fits on first page
  if(offset < PAGE_SIZE && offset + len_string < PAGE_SIZE)
  {
    char* start_next_string = (char*)virtual_address + offset; 
    debug(EXEC, "Write to first page, start from %p, and write %d characters.\n", start_next_string, len_string); 
    memcpy(start_next_string, string, len_string);
  }
  else if(offset > PAGE_SIZE && offset + len_string > PAGE_SIZE)
  {
    char* start_next_string = (char*)virtual_address_2 + offset - PAGE_SIZE;  
    debug(EXEC, "Write to second page, start from %p, and write %d characters.\n", start_next_string, len_string); 
    memcpy(start_next_string, string, len_string);
  }
  else
  {
    //first page
    size_t len_first_page = PAGE_SIZE - offset;
    char* start_next_string1 = (char*)virtual_address + offset;  
    memcpy(start_next_string1, string, len_first_page);

    //second page
    size_t len_second_page = len_string - len_first_page;
    char* start_next_string2 = (char*)virtual_address_2;  
    memcpy(start_next_string2, (char*)((size_t)string + len_first_page), len_second_page);
    
    debug(EXEC, "Write to first and second page, first: %p,%ld, second: %p,%ld.\n", start_next_string1, len_first_page, start_next_string2, len_second_page); 
  }
}

int UserProcess::execvProcess(const char *path, char *const argv[])
{
  UserThread& currentUserThread = *((UserThread*)currentThread);

  if(!Syscall::check_parameter((size_t)path, false) || !Syscall::check_parameter((size_t)argv, false))
  {
    return -1;
  }

  int argc = 0;
  int exec_array_offset = 0;
  //checks if the parameters are currently valid userspace addresses and if there is enough space on two pages
  if(!check_parameters_for_exec(argv, argc, exec_array_offset))
  {
    return -1;
  }


  int path_len = strlen(path)+1;
  char kernel_path[path_len];
  memcpy(kernel_path, path, path_len);


  //check if path exist
  int32 execv_fd = VfsSyscall::open(kernel_path, O_RDONLY);
  if(execv_fd < 0)
  {
    VfsSyscall::close(execv_fd);
    return -1;
  }
  VfsSyscall::close(execv_fd);


  //kill all other threads and wait until all of them have unmapped there stacks
  threads_lock_.acquire();
  currentUserThread.cancel_state_type_lock_.acquire();
  if(((UserThread*)currentThread)->cancel_type_ == PTHREAD_CANCEL_EXIT)
  {
    threads_lock_.release();
    currentUserThread.cancel_state_type_lock_.release();
    return -1;
  }
  currentUserThread.cancel_state_type_lock_.release();

  cancelAllOtherThreads();
  currentUserThread.send_kill_notification();
  one_thread_left_ = (threads_.size() > 1) ? false : true;
  threads_lock_.release();
  waitForThreadsToDie();

  argc = 0;
  exec_array_offset = 0;
  check_parameters_for_exec(argv, argc, exec_array_offset);


  //allocate one (or two) physical pages for the arguments
  size_t page_for_args = PageManager::instance()->allocPPN();
  size_t next_page_for_args = NULL;

  if(exec_array_offset + (argc + 1) * POINTER_SIZE > PAGE_SIZE)
  {
    next_page_for_args = PageManager::instance()->allocPPN();
  }


  size_t offset = 0;
  size_t virtual_address_args = USER_BREAK - 2 * PAGE_SIZE;

  
  for(int i = 0; i < argc; i++)
  {
    int len_string = strlen(argv[i])+1;
    char* start_next_array_element = (char*)((size_t)exec_array_offset + i * POINTER_SIZE);
    //write next argument to page
    UserProcess::write_to_page(page_for_args, next_page_for_args, offset, argv[i], len_string);

    //write the pointer to the next argument to page 
    UserProcess::write_to_page(page_for_args, next_page_for_args, (size_t)start_next_array_element, (char*)&virtual_address_args , POINTER_SIZE);
    
    offset += strlen(argv[i]) + 1;
    virtual_address_args += strlen(argv[i]) + 1;
  }
  if(argc > 0)
  { 
    char* start_next_array_element = (char*)((size_t)exec_array_offset + argc * POINTER_SIZE);
    char* null = NULL;

    //nullterminate the array of pointers
    UserProcess::write_to_page(page_for_args, next_page_for_args, (size_t)start_next_array_element, (char*)&null , POINTER_SIZE);
  }
  
  execv_fd = VfsSyscall::open(kernel_path, O_RDONLY);

  //delete the archmemory of current thread besides first page of current stack
  loader_->arch_memory_.deleteEverythingExecpt(currentUserThread.vpn_stack_);
  size_t old_cr3 = currentThread->user_registers_->cr3;
  //replace the loader of the current binary with the loader of the new binary
  loader_->replaceLoader(execv_fd);
  VfsSyscall::close(fd_);
  fd_ = execv_fd;

  //create fresh user registers for the thread (only leave the cr3 the same)
  ArchThreads::createUserRegisters(currentThread->user_registers_, loader_->getEntryFunction(), (void*) currentUserThread.user_stack_ptr_, currentThread->getKernelStackStartPointer());
  currentThread->user_registers_->cr3 = old_cr3;

  //set argc and argv
  currentThread->user_registers_->rdi = argc;
  currentThread->user_registers_->rsi = USER_BREAK - 2 * PAGE_SIZE + exec_array_offset;
  
  //map the argument page(s)
  loader_->arch_memory_.lock_.acquire();
  bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 2 , page_for_args, 1);
  assert(vpn_mapped &&  "Virtual page already mapped.");
  if(next_page_for_args)
  {
    bool vpn_mapped = loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1 , next_page_for_args, 1);
    assert(vpn_mapped && "Virtual page already mapped.");

  }
  loader_->arch_memory_.lock_.release();
  return 0;
}


bool UserProcess::check_parameters_for_exec(char *const argv[], int& argc, int& array_offset)
{
  int space_left = 2 * PAGE_SIZE;
  bool finished = false;

  //go through all arguments, check if the are valid and if there is enough space
  while(!finished)
  {
    if(!Syscall::check_parameter((size_t)argv[argc], true))
    {
      debug(EXEC, "Execv: parameters not in userspace\n");
      return false;
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
      debug(EXEC, "Execv: no space left\n");
      return false;
    }
  }
  return true;
}

void UserProcess::waitForThreadsToDie()
{
  //wait for the other threads to die
  one_thread_left_lock_.acquire();
  while(!one_thread_left_)
  {
    one_thread_left_condition_.wait();
  }
  one_thread_left_lock_.release();

  thread_retval_map_.clear();

  assert(threads_.size() == 1 && "Not all threads removed from threads_");
  assert(thread_retval_map_.size() == 0 && "There are still values in retval map");

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


ustl::string UserProcess::str() const {
  ustl::ostringstream oss;
  oss << "Process ID: " << pid_;
  return oss.str();
}

long int UserProcess::waitProcess(long int pid, int* status, int options)
{
  ProcessRegistry::instance()->process_exit_status_map_lock_.acquire();
  debug(WAIT_PID, "--------------pid %zu status %p option %d\n", pid, status, options);

  while (ProcessRegistry::instance()->process_exit_status_map_.find(pid) == ProcessRegistry::instance()->process_exit_status_map_.end())
  {
    debug(WAIT_PID, "--------------WAITING\n");
    ProcessRegistry::instance()->process_exit_status_map_condition_.wait();
  }
  debug(WAIT_PID, "--------------DONE WAITING\n");

  int status_tmp = (int)ProcessRegistry::instance()->process_exit_status_map_[pid]; //set
  //at this point the waited process is already(should die) so delete if from map
  ProcessRegistry::instance()->process_exit_status_map_.erase(pid);
  ProcessRegistry::instance()->process_exit_status_map_lock_.release();

  *status = status_tmp;

  return pid;
}