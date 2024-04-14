#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"
#include "Pipe.h"
#include "FileDescriptorManager.h"
#include "FileOperations.h"
#include "uvector.h"
#include "Mutex.h"
#include "Loader.h"
#include "umap.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "ArchThreads.h"

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
      return_value = -1;
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
    case sc_clock:
      return_value = clock();
      break;
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
}

/** TODO: This TODO is only relevant when implementing reserving more stacks for thread. Dont delete
 * this regards to stack reservation for userspace lock mechanism. if the top of stack is 0, 
 * this means this is the first stack request. Make the top_stack to point to itself and give this
 * pointer to child stack's top_stack as well. Calculation to get top of stack is in pthread.c in userspace
*/

uint32 Syscall::pipe(int file_descriptor_array[2])
{
  debug(SYSCALL, "Syscall::pipe called\n");

  Pipe* new_pipe = new Pipe();

  int read_fd = FileDescriptorManager::getInstance().allocateDescriptor(new_pipe, READ);
  int write_fd = FileDescriptorManager::getInstance().allocateDescriptor(new_pipe, WRITE);


  if (read_fd == -1 || write_fd == -1) {
    debug(SYSCALL, "Syscall::pipe failed to allocate file descriptors\n");
    delete new_pipe;
    return -1;
  }

  file_descriptor_array[0] = read_fd;
  file_descriptor_array[1] = write_fd;

  debug(SYSCALL, "Syscall::pipe allocated file descriptors: read_fd = %d, write_fd = %d\n", read_fd, write_fd);

  return 0;
}


// TODOs: handle return value when fork fails, handle how process exits correctly after fork
uint32 Syscall::forkProcess()
{
  debug(FORK, "Syscall::forkProcess: start focking \n");
  UserProcess* parent = ((UserThread*) currentThread)->process_;
  UserProcess* child = new UserProcess(*parent);

  if (!child)
  {
    debug(SYSCALL, "Syscall::forkProcess: fock failed \n");
    return -1;
  }
  else
  {
    debug(SYSCALL, "Syscall::forkProcess: fock done with return (%d) \n", (uint32) currentThread->user_registers_->rax);
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
  int rv = ((UserThread*) currentThread)->createThread(thread, start_routine, wrapper_address, arg, attr);
  return rv;
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
  debug(SYSCALL, "Syscall::EXIT: Thread (%zu) called exit_code: %zdd\n", currentThread->getTID(), exit_code);
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  current_process.exitProcess(exit_code);
  assert(false && "This should never happen");
}

int Syscall::execv(const char *path, char *const argv[])
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;
  return current_process.execvProcess(path, argv);
}


size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    return -1U;
  }

  if (fd >=3 && fd <= 1024) {  // Adjust this condition based on your actual file descriptor range for pipes.
    Pipe* pipe = static_cast<Pipe *>(FileDescriptorManager::getInstance().getAssociatedObject((int)fd));
    for (size_t i = 0; i < size; i++) {
      pipe->write(reinterpret_cast<char *>(buffer)[i]);
    }
    return size;
  }
  size_t num_written = 0;

  if (fd == fd_stdout) //stdout
  {
    //debug(SYSCALL, "Syscall::write: %.*s\n", (int)size, (char*) buffer);
    kprintf("%.*s", (int)size, (char*) buffer);
    num_written = size;
  }
  else
  {
    num_written = VfsSyscall::write(fd, (char*) buffer, size);
  }
  return num_written;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
  {
    return -1U;
  }

  size_t num_read = 0;

  if (fd >= 3 && fd <= 1024) {  //unsure about number
    Pipe* pipe = static_cast<Pipe *>(FileDescriptorManager::getInstance().getAssociatedObject((int)fd));
    size_t i = 0;
    char c;

    while (i < count && pipe->read(c)) {
      reinterpret_cast<char *>(buffer)[i++] = c;
    }
    return i;
  }
  else if (fd == fd_stdin)
  {
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", (int)num_read, (char*) buffer);
    return num_read;
  }

  num_read = VfsSyscall::read(fd, (char*) buffer, count);
  return num_read;
}

size_t Syscall::close(size_t fd)
{
  return VfsSyscall::close(fd);
}

size_t Syscall::open(size_t path, size_t flags)
{
  if (path >= USER_BREAK)
  {
    return -1U;
  }
  return VfsSyscall::open((char*) path, flags);
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
  if(state != CancelState::PTHREAD_CANCEL_DISABLE && state != CancelState::PTHREAD_CANCEL_ENABLE)
  {
    debug(SYSCALL, "Syscall::pthread_setcancelstate: given state is not recognizable\n");
    return -1;
  }
  debug(SYSCALL, "Syscall::pthread_setcancelstate: thread (%zu) is setted cancel state to (%d)\n", currentThread->getTID(), state);
  ((UserThread*) currentThread)->cancel_state_type_lock_.acquire();
  *oldstate = (int) ((UserThread*) currentThread)->cancel_state_;

  ((UserThread*) currentThread)->cancel_state_ = (CancelState)state;

  debug(SYSCALL, "current state %s, previous state %s\n",
        state == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED",
        *oldstate == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED");
  ((UserThread*) currentThread)->cancel_state_type_lock_.release();
  return 0;
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
    if(type != CancelType::PTHREAD_CANCEL_ASYNCHRONOUS && type != PTHREAD_CANCEL_DEFERRED)
    {
        debug(SYSCALL, "Syscall::pthread_setcanceltype: given type is not recognizable\n");
        return -1;
    }
    ((UserThread*) currentThread)->cancel_state_type_lock_.acquire();
    CancelType previous_type = ((UserThread*) currentThread)->cancel_type_;
    *oldtype = (int)previous_type;
    ((UserThread*) currentThread)->cancel_type_ = (CancelType)type;

    debug(SYSCALL, "current type %s, previous type %s\n",
          type == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS",
          *oldtype == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS");
    ((UserThread*) currentThread)->cancel_state_type_lock_.release();
    return 0;
}

unsigned int Syscall::clock(void)
{
  UserThread& currentUserThread = *((UserThread*)currentThread);
  UserProcess& current_process = *currentUserThread.process_;

  uint64_t timestamp_fs = Scheduler::instance()->timestamp_fs_;

  uint64_t current_time_stamp = get_current_timestamp_64_bit();
  uint64_t clock = current_process.clock_ + current_time_stamp - current_process.tsc_start_scheduling_;

  uint64_t clock_in_femtoseconds = (uint64_t)clock * timestamp_fs;

  if(clock_in_femtoseconds / timestamp_fs != clock)
  {
    //overflow occured - which also means the number is to big to represent as unsigned int
    return -1;
  }

  uint64_t clock_in_microseconds = (clock_in_femtoseconds / (uint64_t)1000000000); 

  if(clock_in_microseconds > BIGGEST_UNSIGNED_INT)
  {
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

