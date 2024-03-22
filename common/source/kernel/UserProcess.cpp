#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
//#include "PageManager.h"
#include "Scheduler.h"

//#include "ArchInterrupts.h"



/**
 * @class UserProcess
 *
 * The UserProcess class represents a user process in the operating system.
 *
 * Usage:
 * 1. Create an instance of the UserProcess class using the constructor.
 * 2. The constructor initializes the UserProcess object by opening the specified file and loading it into memory.
 * 3. If loading the file and initializing the process fails, an error message is printed and the process is not created.
 * 4. Otherwise, a UserThread object is created and added to the threads_ vector.
 *
 * Note:
 * - If the process is forked, the processStart() method from ProcessRegistry should also be called.
 * - The kill() method belongs to the Thread class, not to the UserProcess class, so its implementation is not included here.
 */
UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, uint32 terminal_number)
    : fd_(VfsSyscall::open(filename, O_RDONLY)), loader_(nullptr), working_dir_(fs_info), tid_counter_(1),
      filename_(filename), terminal_number_(terminal_number) //local_fd_table_()
{
  ProcessRegistry::instance()->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    //kill();           // This belongs to Thread, not sure what to do here
    return;
  }
  
  threads_.push_back(new UserThread(fs_info, filename, Thread::USER_THREAD, terminal_number, loader_, this, 
                                    tid_counter_, nullptr, nullptr, nullptr));
  tid_counter_++;
  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());
}

UserProcess::~UserProcess()
{
  if (fd_ > 0)
    VfsSyscall::close(fd_);

  delete working_dir_;
  working_dir_ = nullptr;


  /////
  local_fd_table_.closeAllFileDescriptors();
  /////



  ProcessRegistry::instance()->processExit();
}

/**
 * Creates a new user thread.
 *
 * @param func Pointer to the function to be executed by the thread
 * @param para Pointer to the parameter to be passed to the function
 * @param tid Pointer to store the thread ID
 * @param pcreate_helper Pointer to additional helper data for thread creation
 */
void UserProcess::createUserThread(void* func, void* para, void* tid, void* pcreate_helper)
{
  debug(USERPROCESS, "UserProcess::createUserThread: func (%p), para (%zu) \n", func, (size_t) para);
  auto* new_thread = new UserThread(working_dir_, filename_, Thread::USER_THREAD, terminal_number_, loader_,
                                          ((UserThread*) currentThread)->process_, tid_counter_, func, para, pcreate_helper);
  threads_.push_back(new_thread);
  *((unsigned long*) tid) = (unsigned long) tid_counter_;
  tid_counter_++;

  debug(USERPROCESS, "UserProcess::createUserThread: Adding new thread to scheduler\n");
  Scheduler::instance()->addNewThread(new_thread);
}


/**
 * @brief Retrieve the UserThread object with the given thread ID (tid).
 *
 * This function loops through the internal vector of UserThread objects (threads_)
 * and checks if each UserThread object's thread ID matches the given parameter (tid).
 * If a match is found, the corresponding UserThread object is returned.
 *
 * @param tid The thread ID to search for.
 * @return UserThread* A pointer to the UserThread object with the given tid, or nullptr if not found.
 */
UserThread* UserProcess::getUserThread(size_t tid)
{
  for (auto & thread : threads_)
  {
    if (thread->getTID() == tid)
      return thread;
  }
  return nullptr;
}

/**
 * Open a file with the given path and mode.
 *
 * @param path The path of the file to open.
 * @param mode The mode to open the file with.
 *
 * @return The local file descriptor of the opened file, or -1 if an error occurred.
 */
[[maybe_unused]] size_t UserProcess::openFile(const ustl::string& path, uint32_t mode) {
  debug(USERPROCESS, "openFile: Attempting to open file: %s with mode: %u\n", path.c_str(), mode);

  int global_fd_id = VfsSyscall::open(path, mode);
  if (global_fd_id < 0) {
    debug(USERPROCESS, "openFile: Failed to open global file descriptor. VfsSyscall::open returned: %d\n", global_fd_id);
    return -1;
  }

  FileDescriptor* global_fd = global_fd_list.getFileDescriptor(global_fd_id);
  if (!global_fd) {
    debug(USERPROCESS, "openFilme: Failed to get global file descriptor from global_fd_list. getFileDescriptor returned NULL.\n");
    return -1;
  }

  LocalFileDescriptor* local_fd = local_fd_table_.createLocalFileDescriptor(global_fd, mode, 0);
  if (!local_fd) {
    debug(USERPROCESS, "openFile: Failed to create local file descriptor. createLocalFileDescriptor returned NULL.\n");
    VfsSyscall::close(global_fd_id);
    return -1;
  }

  debug(USERPROCESS, "openFile: Successfully opened file: %s with mode: %u. Local FD: %zu\n", path.c_str(), mode, local_fd->getLocalFD());
  return local_fd->getLocalFD();
}
