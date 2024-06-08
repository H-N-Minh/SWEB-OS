#include "offsets.h" //
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"
#include "Pipe.h"
#include "FileOperations.h"
#include "uvector.h"
#include "Mutex.h"
#include "Loader.h"
#include "umap.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "ArchThreads.h"
#include "UserSpaceMemoryManager.h"
#include "ProcessRegistry.h"
#include "ScopeLock.h"
#include "SwappingManager.h"
#include "SharedMemManager.h"

#define BIGGEST_UNSIGNED_INT 4294967295


size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;
  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline)) // no debug print because these might occur very often
  {
    debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
          syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  }

  UserThread& currentUserThread = *((UserThread*)currentThread);
  currentUserThread.cancel_state_type_lock_.acquire();
  //Cancelation point: if the cancelstate is enabled and the thread recieved a cancelation request the thread gets canceled
  if(currentUserThread.wants_to_be_canceled_)
  {
    if(currentUserThread.cancel_type_ == PTHREAD_CANCEL_EXIT || currentUserThread.cancel_state_ == PTHREAD_CANCEL_ENABLE)
    {
      currentUserThread.cancel_state_type_lock_.release();
      pthreadExit((void*)-1);
    }
  }
  currentUserThread.cancel_state_type_lock_.release();

  switch (syscall_number)
  {
    case sc_sched_yield:
      Scheduler::instance()->yield();
      break;
    case sc_createprocess:
      return_value = createprocess(arg1, arg2);
      break;
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value = write(arg1, arg2, arg3);
      break;
    case sc_read:
      return_value = read(arg1, arg2, arg3);
      break;
    case sc_open:
      return_value = open(arg1, arg2);
      break;
    case sc_close:
      return_value = close(arg1);
      break;
    case sc_lseek:
      return_value = lseek(arg1, arg2, arg3);
      break;
    case sc_outline:
      outline(arg1, arg2);
      break;
    case sc_trace:
      trace();
      break;
    case sc_pseudols:
      pseudols((const char*) arg1, (char*) arg2, arg3);
      break;
    case sc_threadcount:
      return_value = get_thread_count();
      break;
    case sc_pthread_self:
      return_value = pthreadSelf();
      break;
    case sc_pthread_create:
      return_value = pthreadCreate((size_t*)arg1, (unsigned int*) arg2, (void*) arg3, (void*) arg4, (void*)arg5);
      break;
    case sc_pthread_exit:
      pthreadExit((void*)arg1);
      break;
    case sc_pthread_join:
      return_value = pthreadJoin((size_t)arg1, (void **)arg2);
      break;
    case sc_pthread_detach:
      return_value = pthreadDetach((size_t)arg1);
      break;
    case sc_pthread_cancel:
      return_value = pthreadCancel((size_t)arg1);
      break;
    case sc_sleep:
      return_value = sleep((unsigned int)arg1);
      break;
    case sc_execv:
      return_value = execv((const char *)arg1, (char *const*)arg2);
      break;
    case sc_pthread_setcancelstate:
      return_value = pthread_setcancelstate((int)arg1, (int *)arg2);
      break;
    case sc_pthread_setcanceltype:
      return_value = pthread_setcanceltype((int)arg1, (int *)arg2);
      break;
    case sc_pthread_testcancel:
      break;
    case sc_fork:
      return_value = forkProcess();
      break;
    case sc_pipe:
      return_value = pipe((int*) arg1);
      break;
    case sc_dup:
      return_value = dup((int) arg1);
      break;
    case sc_clock:
      return_value = clock();
      break;
    case sc_tortillas_bootup:   // needed for test system Tortillas
      break;
    case sc_tortillas_finished:   // needed for test system Tortillas
      break;
    case sc_sbrk:
      return_value = sbrkMemory((ssize_t)arg1);
      break;
    case sc_brk:
      return_value = brkMemory(arg1);
      break;
    case sc_wait_pid:
      return_value = wait_pid((int)arg1, (int*)arg2, arg3);
      break;
    case sc_getIPTInfos:
      getIPTInfos();
      break; 
    case sc_assertIPT:
      assertIPT();
      break;
    case sc_setPRA:
      setPraType(arg1);
      break;
    case sc_getPRAstats:
      return_value = getPRAstats((int*)arg1, (int*)arg2);
      break;
    case sc_checkRandomPRA:
      checkRandomPRA();
      break;
    case sc_mmap:
      return_value = mmap(arg1, arg2);
      break;
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
}

int Syscall::mmap(size_t para, size_t retval)
{
  assert(para && "Syscall::mmap: arg1 is null\n");
  mmap_params_t* params = (mmap_params_t*) para;
  void* start = params->start;
  size_t length = params->length;
  int prot = params->prot;
  int flags = params->flags;
  int fd  = params->fd;
  off_t offset = params->offset;
  debug(MINH, "Syscall::mmap: start: %p, length: %zu, prot: %d, flags: %d, fd: %d, offset: %ld\n",start, length, prot, flags, fd, offset);
  // error checking the para
  if (start != (void*) 0 || offset != 0)
  {
    debug(ERROR_DEBUG, "Syscall::mmap: start and offset is not implemented\n");
    return -1;
  }
  if (fd < 0)
  {
    debug(ERROR_DEBUG, "Syscall::mmap: invalid fd\n");
    return -1;
  }
  
  SharedMemManager* smm = ((UserThread*)currentThread)->process_->user_mem_manager_->shared_mem_;
  void* ret = smm->mmap(params);
  *(size_t*) retval = (size_t) ret;
  debug(MINH, "Syscall::mmap: return value: %p\n", ret);

  return 0;
}

l_off_t Syscall::lseek(size_t fd, l_off_t offset, uint8 whence) {
  debug(Fabi, "Syscall::lseek: Attempting to do lseek on fd: %zu\n", fd);

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  l_off_t position = -1;

  {
    ScopeLock lock(current_process.localFileDescriptorTable.lfds_lock_);

    LocalFileDescriptor* localFileDescriptor = current_process.localFileDescriptorTable.getLocalFileDescriptor(fd);
    if (localFileDescriptor == nullptr) {
      debug(SYSCALL, "Syscall::lseek - Invalid local file descriptor: %zu\n", fd);
      return -1;
    }

    size_t global_fd = localFileDescriptor->getGlobalFileDescriptor()->getFd();
    FileDescriptor* file_descriptor = VfsSyscall::getFileDescriptor(global_fd);
    assert(file_descriptor != nullptr && "File descriptor pointer is null");
    debug(FILEDESCRIPTOR, "Syscall::lseek: Global FD = %u; RefCount = %d\n", file_descriptor->getFd(), file_descriptor->getRefCount());

    position = VfsSyscall::lseek(global_fd, offset, whence);
    debug(Fabi, "Syscall::lseek: Positioned at: %zd for global fd: %zu\n", position, global_fd);
  }

  return position;
}



size_t Syscall::brkMemory(size_t new_brk_addr)
{
  debug(SBRK, "Syscall::brkMemory: brk called with address %p. Checking if addr is valid \n", (void*) new_brk_addr);

  UserSpaceMemoryManager* heap_manager = ((UserThread*) currentThread)->process_->user_mem_manager_;


  heap_manager->current_break_lock_.acquire();
  int successly_brk = heap_manager->brk(new_brk_addr);
  heap_manager->current_break_lock_.release();

  return successly_brk;
}

size_t Syscall::sbrkMemory(ssize_t size)
{
  debug(SBRK, "Syscall::sbrkMemory: sbrk called\n");
  UserSpaceMemoryManager* heap_manager = ((UserThread*) currentThread)->process_->user_mem_manager_;

  debug(SBRK, "Syscall::sbrkMemory: calling sbrk from heap manager and check if its valid\n");


  heap_manager->current_break_lock_.acquire();
  void* old_break = heap_manager->sbrk(size);
  heap_manager->current_break_lock_.release();

  return (size_t)old_break;
}


uint32 Syscall::forkProcess()
{
  debug(FORK, "Syscall::forkProcess: start forking \n");
  UserProcess* parent = ((UserThread*) currentThread)->process_;
  UserProcess* child = new UserProcess(*parent);

  if (!child)
  {
    debug(SYSCALL, "Syscall::forkProcess: fork failed \n");
    return -1;
  }
  else
  {
    debug(SYSCALL, "Syscall::forkProcess: fork done with return (%d) \n", (uint32) currentThread->user_registers_->rax);
    return (uint32) currentThread->user_registers_->rax;
  }
}


void Syscall::pthreadExit(void* value_ptr)
{
  debug(SYSCALL, "Syscall::pthreadExit: called, value_ptr: %p.\n", value_ptr);
  ((UserThread*)currentThread)->exitThread(value_ptr);

  assert(false && "This should never happen");
}


int Syscall::pthreadJoin(size_t thread_id, void**value_ptr)
{
  debug(SYSCALL, "Syscall:pthreadJoin: called, thread_id: %zu and %p\n", thread_id, value_ptr);

  if(!check_parameter((size_t)value_ptr, true) || (currentThread->getTID() == thread_id))
  {
    debug(USERTHREAD, "UserThread:pthreadJoin: Thread tries to join itself or invalid value_ptr.\n");
    return -1;
  }
  
  return ((UserThread*)currentThread)->joinThread(thread_id, value_ptr);
}


int Syscall::pthreadDetach(size_t thread_id)
{
  debug(SYSCALL, "Syscall:pthreadDetach: called, thread_id: %zu\n", thread_id);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  current_process.threads_lock_.acquire();
  int rv = currentUserThread.detachThread(thread_id);
  current_process.threads_lock_.release();
  return rv;
}

int Syscall::pthreadCreate(size_t* thread, unsigned int* attr, void* start_routine, void* arg, void* wrapper_address)
{
  debug(SYSCALL, "Syscall::pthreadCreate pthreadCreated called\n");
  int rv = ((UserThread*) currentThread)->createThread(thread, start_routine, wrapper_address, arg, (pthread_attr_t*)attr);
  return rv;
}

size_t Syscall::pthreadSelf()
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  return currentUserThread.getTID();
}

int Syscall::pthreadCancel(size_t thread_id)
{
  debug(SYSCALL, "Syscall::pthreadCancel: called with thread_id %ld.\n",thread_id);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  current_process.threads_lock_.acquire();
  int rv = currentUserThread.cancelThread(thread_id);
  current_process.threads_lock_.release();

  return rv;
}



void Syscall::exit(size_t exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: Thread (%zu) called exit_code: %zd\n", currentThread->getTID(), exit_code);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  if (exit_code != 69)
  {
    debug(SYSCALL, "Tortillas test system received exit code: %zd\n", exit_code); // dont delete
  }

  current_process.exitProcess(exit_code);
  assert(false && "This should never happen");
}


int Syscall::execv(const char *path, char *const argv[])
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  return current_process.execvProcess(path, argv);
}


size_t Syscall::dup(int file_descriptor) {
  debug(SYSCALL, "Syscall::dup called to duplicate fd %d\n", file_descriptor);

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

  LocalFileDescriptor* originalLFD = lfdTable.getLocalFileDescriptor(file_descriptor);
  if(originalLFD == nullptr) {
    debug(SYSCALL, "Syscall::dup: original file descriptor not found\n");
    return -1;
  }

  int newDescriptor = lfdTable.createLocalFileDescriptor(
      originalLFD->getGlobalFileDescriptor(),
      originalLFD->getMode(),
      originalLFD->getOffset(),
      originalLFD->getType()
  )->getLocalFD();

  debug(SYSCALL, "Syscall::dup: returning the duplicated fd %d\n", newDescriptor);
  return newDescriptor;
}

uint32 Syscall::pipe(int file_descriptor_array[2]) {
  assert(file_descriptor_array != nullptr && "file_descriptor_array is null");  // Parameter check

  debug(PIPE, "Syscall::pipe called\n");

  Pipe* new_pipe = new Pipe(nullptr, FileDescriptor::FileType::PIPE);
  int res = global_fd_list.add(reinterpret_cast<FileDescriptor *>(new_pipe));
  if (res != 0) {
    debug(PIPE, "Failed to add new_pipe to fd list\n");
    delete new_pipe;
    return -1;
  }

  debug(PIPE, "Syscall::pipe: new_pipe = %p\n", new_pipe);

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

  size_t read_fd = lfdTable.createLocalFileDescriptor(
      reinterpret_cast<FileDescriptor *>(new_pipe), O_RDONLY, 0, ::FileType::PIPE)->getLocalFD();

  debug(PIPE, "Syscall::pipe: read_fd = %zu\n", read_fd);

  size_t write_fd = lfdTable.createLocalFileDescriptor(
      reinterpret_cast<FileDescriptor *>(new_pipe), O_WRONLY, 0, ::FileType::PIPE)->getLocalFD();

  debug(PIPE, "Syscall::pipe: write_fd = %zu\n", write_fd);

  file_descriptor_array[0] = static_cast<int>(read_fd);
  file_descriptor_array[1] = static_cast<int>(write_fd);

  debug(PIPE, "Syscall::pipe: returning %zu, %zu\n", read_fd, write_fd);
  return 0;
}



size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{

  debug(SYSCALL, "Syscall::write: Writing to fd: %zu with buffer size: %zu\n", fd, size);
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    debug(SYSCALL, "Syscall::write: Buffer exceeds USER_BREAK\n");
    return -1U;
  }

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

  debug(SYSCALL, "Syscall::write: Current Process: %s\n", current_process.str().c_str());

  lfdTable.lfds_lock_.acquire();

  LocalFileDescriptor* localFileDescriptor = current_process.localFileDescriptorTable.getLocalFileDescriptor(fd);
  debug(SYSCALL, "Syscall::write: localFileDescriptor for fd %zu: %p\n", fd, (void*)localFileDescriptor);

  if (fd == fd_stdout)
  {
    lfdTable.lfds_lock_.release();
    debug(SYSCALL, "Syscall::write: Writing to stdout\n");
    kprintf("%.*s", (int)size, (char*) buffer);
    return size;
  }
  else if (localFileDescriptor != nullptr) {
    FileDescriptor *global_fd_obj = localFileDescriptor->getGlobalFileDescriptor();
    assert(global_fd_obj != nullptr && "Global file descriptor pointer is null");

    debug(SYSCALL, "Syscall::write: Global FD = %u; RefCount = %d\n", global_fd_obj->getFd(), global_fd_obj->getRefCount());

    if (global_fd_obj->getType() == FileDescriptor::FileType::PIPE)
    {
      debug(PIPE, "Syscall::write: Attempting to write to pipe: %p\n", (void*)global_fd_obj);
      Pipe* pipeObj = static_cast<Pipe*>(global_fd_obj);
      lfdTable.lfds_lock_.release();
      size_t num_written = pipeObj->write((char*)buffer, size);
      debug(PIPE, "Syscall::write: Wrote %zu bytes to pipe: %p\n", num_written, (void*)pipeObj);
      return num_written;
    } else {
      size_t global_fd = global_fd_obj->getFd();
      size_t num_written = VfsSyscall::write(global_fd, (char *) buffer, size);
      debug(SYSCALL, "Syscall::write: Wrote %zu bytes to global fd: %zu\n", num_written, global_fd);
      lfdTable.lfds_lock_.release();
      return num_written;
    }
  }
  debug(SYSCALL, "Syscall::write: No valid local file descriptor found for fd: %zu\n", fd);
  lfdTable.lfds_lock_.release();
  return -1U;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  debug(SYSCALL, "Syscall::read: Attempting to read from fd: %zu with buffer size: %zu\n", fd, count);
  if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
  {
    debug(SYSCALL, "Syscall::read: Buffer exceeds USER_BREAK\n");
    return -1U;
  }

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

  lfdTable.lfds_lock_.acquire();
  LocalFileDescriptor* localFileDescriptor = current_process.localFileDescriptorTable.getLocalFileDescriptor(fd);

  if (fd == fd_stdin)
  {
    debug(SYSCALL, "Syscall::read: Reading from stdin\n");
    lfdTable.lfds_lock_.release();
    size_t num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: Read %zu bytes from stdin\n", num_read);
    return num_read;
  }
  else if (localFileDescriptor != nullptr) {
    FileDescriptor *global_fd_obj = localFileDescriptor->getGlobalFileDescriptor();
    assert(global_fd_obj != nullptr && "Global file descriptor pointer is null");

    debug(SYSCALL, "Syscall::read: Global FD = %u; RefCount = %d\n", global_fd_obj->getFd(), global_fd_obj->getRefCount());

    if (global_fd_obj->getType() == FileDescriptor::FileType::PIPE){
      debug(PIPE, "Syscall::read: Attempting to read from pipe: %p\n", (void*)global_fd_obj);
      Pipe* pipeObj = static_cast<Pipe*>(global_fd_obj);
      lfdTable.lfds_lock_.release();
      bool success = pipeObj->read((char*)buffer, count);
      lfdTable.lfds_lock_.acquire();
      debug(SYSCALL, "Syscall::read: Read %zu bytes from pipe: %p\n", success? count : 0, (void*)pipeObj);
      lfdTable.lfds_lock_.release();
      return success? count : 0;
    } else {
      size_t global_fd = global_fd_obj->getFd();
      size_t num_read = VfsSyscall::read(global_fd, (char *) buffer, count);
      debug(SYSCALL, "Syscall::read: Read %zu bytes from global fd: %zu\n", num_read, global_fd);
      lfdTable.lfds_lock_.release();
      return num_read;
    }
  }
  debug(SYSCALL, "Syscall::read: No valid local file descriptor found\n");
  lfdTable.lfds_lock_.release();
  return -1U;
}

size_t Syscall::close(size_t fd)
{
  debug(SYSCALL, "Syscall::close: Attempting to close fd: %zu\n", fd);

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;
  lfdTable.lfds_lock_.acquire();

  LocalFileDescriptor* localFileDescriptor = current_process.localFileDescriptorTable.getLocalFileDescriptor(fd);

  if (localFileDescriptor != nullptr)
  {
    FileDescriptor *global_fd_obj = localFileDescriptor->getGlobalFileDescriptor();
    assert(global_fd_obj != nullptr && "Global file descriptor pointer is null");

    int rv = current_process.localFileDescriptorTable.removeLocalFileDescriptor(localFileDescriptor);

    lfdTable.lfds_lock_.release();
    return rv;
  }
  debug(SYSCALL, "Syscall::close: No valid local file descriptor found for fd: %zu\n", fd);
  lfdTable.lfds_lock_.release();
  return -1U;
}

size_t Syscall::open(size_t path, size_t flags)
{
  if (path >= USER_BREAK)
  {
    return -1U;
  }

  debug(SYSCALL, "Syscall::open: Opening path: %s with flags: %zu\n", (char*)path, flags);

  int global_fd = VfsSyscall::open((char*) path, flags);
  if (global_fd < 0) {
    debug(SYSCALL, "Syscall::open: VfsSyscall::open failed\n");
    return -1U;
  }

  debug(SYSCALL, "Syscall::open: Global file descriptor: %d\n", global_fd);

  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

  lfdTable.lfds_lock_.acquire();


  FileDescriptor* globalFileDescriptor = VfsSyscall::getFileDescriptor(global_fd);
  LocalFileDescriptor* localFileDescriptor = current_process.localFileDescriptorTable.createLocalFileDescriptor(globalFileDescriptor, flags, 0, ::FileType::REGULAR);

  debug(SYSCALL, "Syscall::open: Current Process: %s\n", current_process.str().c_str());

  if (localFileDescriptor == nullptr) {
    debug(SYSCALL, "Syscall::open: LocalFileDescriptor creation failed\n");
    lfdTable.lfds_lock_.release();
    return -1U;
  }

  debug(SYSCALL, "Syscall::open: Local file descriptor: %zu\n",
        localFileDescriptor->getLocalFD());
  size_t local_file_descriptor = localFileDescriptor->getLocalFD();
  lfdTable.lfds_lock_.release();
  return local_file_descriptor;
}



void Syscall::outline(size_t port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= USER_BREAK)
  {
    return;
  }
  if (port == 0xe9) // debug port
  {
    writeLine2Bochs((const char*) text);
  }
}

size_t Syscall::createprocess(size_t path, size_t sleep)
{
  // THIS METHOD IS FOR TESTING PURPOSES ONLY AND NOT MULTITHREADING SAFE!
  // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

  // parameter check begin
  if (path >= USER_BREAK)
  {
    return -1U;
  }

  debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%zd\n", (char*) path, sleep);
  ssize_t fd = VfsSyscall::open((const char*) path, O_RDONLY);
  if (fd == -1)
  {
    return -1U;
  }
  VfsSyscall::close(fd);
  // parameter check end

  size_t process_count = ProcessRegistry::instance()->processCount();
  ProcessRegistry::instance()->createProcess((const char*) path);
  if (sleep)
  {
    while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return 0;
}

void Syscall::trace()
{
  currentThread->printBacktrace();
}

uint32 Syscall::get_thread_count() {
  return Scheduler::instance()->getThreadCount();
}



void Syscall::pseudols(const char *pathname, char *buffer, size_t size)
{
  if(buffer && ((size_t)buffer >= USER_BREAK || (size_t)buffer + size > USER_BREAK))
    return;
  if((size_t)pathname >= USER_BREAK)
    return;
  VfsSyscall::readdir(pathname, buffer, size);
}

unsigned int Syscall::sleep(unsigned int seconds)
{
  unsigned int seconds_left = seconds % 1000;
  unsigned int times_thousands_seconds = seconds / 1000;
  uint64_t femtoseconds = (uint64_t)seconds_left * 1000000000000000;
  uint64_t current_time_stamp = get_current_timestamp_64_bit();
  uint64_t timestamp_fs = Scheduler::instance()->timestamp_fs_;
  uint64_t thousand_femtoseconds = 1000000000000000000;
  currentThread->wakeup_timestamp_ = current_time_stamp + (femtoseconds / timestamp_fs) +  times_thousands_seconds * (thousand_femtoseconds / timestamp_fs);

  Scheduler::instance()->yield();

  return 0;
}



bool Syscall::check_parameter(size_t ptr, bool allowed_to_be_null)
{
  if(!allowed_to_be_null && ptr == 0)
  {
    return false;
  }
  if(ptr >= USER_BREAK)
  {
    return false;
  }
  return true;
}



int Syscall::pthread_setcancelstate(int state, int *oldstate)
{
  if(!Syscall::check_parameter((size_t)oldstate, false))
  {
    return -1;
  }
  UserThread& currentUserThread = *((UserThread*)currentThread);
  if(state != CancelState::PTHREAD_CANCEL_DISABLE && state != CancelState::PTHREAD_CANCEL_ENABLE)
  {
    debug(SYSCALL, "Syscall::pthread_setcancelstate: given state is not recognizable\n");
    return -1;
  }
  debug(SYSCALL, "Syscall::pthread_setcancelstate: thread (%zu) is setted cancel state to (%d)\n", currentThread->getTID(), state);
  currentUserThread.cancel_state_type_lock_.acquire();
  int temp_oldstate = (int) currentUserThread.cancel_state_;

  currentUserThread.cancel_state_ = (CancelState)state;

  currentUserThread.cancel_state_type_lock_.release();
  *oldstate = temp_oldstate;
  return 0;
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
  if(!Syscall::check_parameter((size_t)oldtype, false))
  {
    return -1;
  }
  UserThread& currentUserThread = *((UserThread*)currentThread);
  if(type != CancelType::PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED)
  {
    debug(SYSCALL, "Syscall::pthread_setcanceltype: given type is not recognizable\n");
    return -1;
  }
  currentUserThread.cancel_state_type_lock_.acquire();
  CancelType previous_type =  currentUserThread.cancel_type_;
  if(previous_type == PTHREAD_CANCEL_EXIT)
  {
    currentUserThread.cancel_state_type_lock_.release();
    return -1;
  }
  int temp_oldtype = (int)previous_type;
  currentUserThread.cancel_type_ = (CancelType)type;

  currentUserThread.cancel_state_type_lock_.release();
  *oldtype = temp_oldtype;
  return 0;
}

unsigned int Syscall::clock(void)
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  uint64_t timestamp_fs = Scheduler::instance()->timestamp_fs_;
  uint64_t current_time_stamp = get_current_timestamp_64_bit();
  uint64_t clock = current_process.clock_ + (current_time_stamp - current_process.tsc_start_scheduling_);

  uint64_t clock_in_femtoseconds = (uint64_t)clock * timestamp_fs;

  if(clock_in_femtoseconds / timestamp_fs != clock)
  {
    //overflow occured - which also means the number is to big to represent as unsigned int
    debug(SYSCALL, "Syscall::clock - Number too big");
    return -1;
  }

  uint64_t clock_in_microseconds = (clock_in_femtoseconds / (uint64_t)1000000000);

  if(clock_in_microseconds > BIGGEST_UNSIGNED_INT)
  {
    debug(SYSCALL, "Syscall::clock - Number too big");
    return -1;
  }

  return (unsigned int)clock_in_microseconds;
}



uint64_t Syscall::get_current_timestamp_64_bit()
{
  unsigned int edx;
  unsigned int eax;
  asm
      (
      "rdtsc"
      : "=a"(eax), "=d"(edx)
      );
  return ((uint64_t)edx<<32) + eax;
}

long int Syscall::wait_pid(long int pid, int* status, size_t options)
{
  if (pid <= 0 || options != 0)
  {
    return -1;
  }

  UserProcess* current_process = ((UserThread*) currentThread)->process_;
  return current_process->waitProcess(pid, status, options);
}

void addBarChart(ustl::string lines[], int position, int number, ustl::string title)
{
  int bar_size = number/50;

  lines[position - 1].replace(1,title.size(), title);
  for(int i = 1; i <= bar_size; i++)
  {
    lines[position].replace(i,1,"x");
  }
  ustl::string number_as_string = "(=" + ustl::to_string(number) + ")";
  lines[position].replace(bar_size + 2 ,number_as_string.size(),number_as_string);
  
}

void Syscall::getIPTInfos()
{
  

  ustl::string empty_line = "|                                                                         |\n";
  ustl::string first_line = "___________________________________________________________________________\n";
  ustl::string last_line  = "|_________________________________________________________________________|\n";
  ustl::string info_line  = "|                                                x = 50 pages             |\n";
  
  ustl::string lines[20];
  lines[0] = first_line;
  lines[19] = last_line;
  for(int i = 1; i < 19; i++)
  {
    lines[i] = empty_line;
  }
  lines[3] = info_line;
  kprintf("\n");

  int pages_ipt_ram = IPTManager::instance()->getNumPagesInMap(IPTMapType::RAM_MAP);
  addBarChart(lines, 7, pages_ipt_ram, "Pages in RAM-MAP");

  int pages_ipt_disk = IPTManager::instance()->getNumPagesInMap(IPTMapType::DISK_MAP);
  addBarChart(lines, 10, pages_ipt_disk, "Pages in DISK-MAP");

  int total_disk_reads = 2;
  // int total_disk_reads = SwappingManager::instance()->getDiskReads();
  addBarChart(lines, 13, total_disk_reads, "Total disk reads");

  int total_disk_writes = SwappingManager::instance()->getDiskWrites();
  addBarChart(lines, 16, total_disk_writes, "Total disk writes");

  for(int i = 0; i < 20; i++)
  {
     kprintf("%s", lines[i].c_str());
  }
  kprintf("\n\n");
}

void Syscall::assertIPT()
{
  auto* ipt = IPTManager::instance();
  ipt->IPT_lock_.acquire();

  debug(SYSCALL, "Syscall::assertIPT: Checking validity of ram map, disk map, swap metadata\n");
  ipt->checkRamMapConsistency();
  ipt->checkDiskMapConsistency();
  debug(SYSCALL, "Syscall::assertIPT: All IPT looks good\n");

  ipt->IPT_lock_.release();
}

void Syscall::setPraType(size_t type)
{
  debug(SYSCALL, "Syscall::setPraType: Setting PRA type to %s\n", type == 0 ? "RANDOM" : "NFU");
  IPTManager* ipt = IPTManager::instance();
  ipt->IPT_lock_.acquire();
  if(type == PRA_TYPE::NFU)
  {
    ipt->pra_type_ = PRA_TYPE::NFU;
  }
  else if(type == PRA_TYPE::RANDOM)
  {
    ipt->pra_type_ = PRA_TYPE::RANDOM;
  }
  else if(type == PRA_TYPE::SECOND_CHANGE)
  {
    ipt->pra_type_ = PRA_TYPE::SECOND_CHANGE;
  }
  ipt->IPT_lock_.release();
}

int Syscall::getPRAstats(int* hit_count, int* miss_count)
{
  if(!check_parameter((size_t)hit_count, false) || !check_parameter((size_t)miss_count, false))
  {
    debug(USERTHREAD, "Syscall::getPRAstats. Invalid pointers given to store the pra stats\n");
    return -1;
  }
  SwappingThread* swapper = &Scheduler::instance()->swapping_thread_;
  *hit_count = swapper->getHitCount();
  *miss_count = swapper->getMissCount();

  return 0;
}

void Syscall::checkRandomPRA()
{
  IPTManager* ipt = IPTManager::instance();
  ipt->debugRandomGenerator();
}