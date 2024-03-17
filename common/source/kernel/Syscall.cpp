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

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;
  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline)) // no debug print because these might occur very often
  {
    debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
          syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  }

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
      break; // you will need many debug hours if you forget the break
    case sc_pthread_create:
      return_value = createThread((void*) arg1, (void*) arg2, (void*) arg3, (void*) arg4);
      break; // you will need many debug hours if you forget the break
    case sc_pthread_join:
      return_value = joinThread(arg1, (void**) arg2);
      break; // you will need many debug hours if you forget the break
    case sc_pthread_exit:
      return_value = exitThread((void*) arg1);
      break; // you will need many debug hours if you forget the break
    case sc_pipe:
      return_value = pipe((int*) arg1);
      break; // you will need many debug hours if you forget the break
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
}

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


uint32 Syscall::exitThread(void* return_value)
{
  debug(SYSCALL, "Syscall::exitThread: zombie the current thread \n");
  ((UserThread*) currentThread)->setReturnValue(return_value);
  Scheduler::instance()->sleep();
  return 0;
}

// TODO: handle when return_ptr is NULL
uint32 Syscall::joinThread(size_t worker_thread, void **return_ptr)
{
  debug(SYSCALL, "Syscall::joinThread: target_thread (%zu), para (%p) \n", worker_thread, return_ptr);
  UserThread* worker = ((UserThread*) currentThread)->process_->getUserThread(worker_thread);
  assert(worker && "Thread not found in Process's vector");

  worker->getReturnValue(return_ptr, worker);
  debug(Fabi, "GOT RESULT IT IS (%zu) \n", (size_t) *return_ptr);
  return 0;
}

uint32 Syscall::createThread(void* func, void* para, void* tid, void* pcreate_helper)
{
  debug(SYSCALL, "Syscall::createThread: func (%p), para (%zu) \n", func, (size_t) para);
  ((UserThread*) currentThread)->process_->createUserThread(func, para, tid, pcreate_helper);
  return 0;
}

void Syscall::pseudols(const char *pathname, char *buffer, size_t size)
{
  if(buffer && ((size_t)buffer >= USER_BREAK || (size_t)buffer + size > USER_BREAK))
    return;
  if((size_t)pathname >= USER_BREAK)
    return;
  VfsSyscall::readdir(pathname, buffer, size);
}

void Syscall::exit(size_t exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: Thread (%zu) called exit_code: %zd\n", currentThread->getTID(), exit_code);
  UserProcess* process = ((UserThread*) currentThread)->process_;

  size_t vector_size = process->threads_.size();
  for (size_t i = 0; i < vector_size; ++i)
  {
    if(process->threads_[i] != currentThread)
    {
      process->threads_[i]->kill();
      i--;
      vector_size--;
    }
  }

  // Scheduler::instance()->printThreadList();
  delete process;
  ((UserThread*) currentThread)->process_ = 0;
  currentThread->kill();
  assert(false && "This should never happen");
}

size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    return -1U;
  }

  size_t num_written = 0;

  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL, "Syscall::write: %.*s\n", (int)size, (char*) buffer);
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

  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", (int)num_read, (char*) buffer);
  }
  else
  {
    num_read = VfsSyscall::read(fd, (char*) buffer, count);
  }
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