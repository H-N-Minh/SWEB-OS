#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"
#include "uvector.h"

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
      break;
    case sc_pthread_create:
      return_value = pthread_create((size_t*)arg1, (unsigned int*)arg2, (void *(*)(void*))arg3, (void*)arg4, (void *(*)())arg5);
      break;
    case sc_pthread_exit:
      pthread_exit((void*)arg1);
      break;  // you will need many debug hours if you forget the break

    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
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
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %zd\n", exit_code);
  ustl::vector<UserThread*> threads_of_process = static_cast<UserThread*>(currentThread)->process_->threads_;
  ustl::vector<UserThread*>::iterator iterator = ustl::find(threads_of_process.begin(), threads_of_process.end(), static_cast<UserThread*>(currentThread));
  size_t number_of_threads_in_process = threads_of_process.size();
  threads_of_process.erase(iterator);
  size_t number_of_threads_in_process_after_removing_current_thread = threads_of_process.size();
  assert(number_of_threads_in_process - number_of_threads_in_process_after_removing_current_thread == 1 && "Current thread was not removed from threadlist.");
  debug(SYSCALL, "Process has currently %ld thread in his list.\n", threads_of_process.size());
  for (auto& thread : threads_of_process)
  {
    assert(thread != static_cast<UserThread*>(currentThread) && "Current thread needs to be killed last.");
    thread->kill();
  }
  delete static_cast<UserThread*>(currentThread)->process_;
  static_cast<UserThread*>(currentThread)->process_ = 0;
  //static_cast<UserThread*>(currentThread)->process_->to_be_destroyed_ = true;  //123
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

int Syscall::pthread_create(size_t* thread, unsigned int* attr, void *(*start_routine)(void*), void* arg, void *(*wrapper_address)())         
{
  //TODO: check arguments
  if(!(check_parameter((size_t)thread) && check_parameter((size_t)attr, true) && check_parameter((size_t)start_routine) && check_parameter((size_t)arg, true) && check_parameter((size_t)wrapper_address)))
  {
    return -1;
  }
  debug(SYSCALL, "Unused: Thread %p, Attribute %p\n", thread, attr);       //TODO
  int rv = static_cast<UserThread*>(currentThread)->process_->add_thread(start_routine, wrapper_address, arg);
  return rv;
}

void Syscall::pthread_exit(void* value_ptr){
  //TODO: check arguments
  debug(SYSCALL, "Return_value of thread was, %ld\n",(size_t)value_ptr);
  ustl::vector<UserThread*> threads_of_process = static_cast<UserThread*>(currentThread)->process_->threads_;
  ustl::vector<UserThread*>::iterator iterator = ustl::find(threads_of_process.begin(), threads_of_process.end(), static_cast<UserThread*>(currentThread));
  size_t number_of_threads_in_process = threads_of_process.size();
  threads_of_process.erase(iterator);
  size_t number_of_threads_in_process_after_removing_current_thread = threads_of_process.size();
  assert(number_of_threads_in_process - number_of_threads_in_process_after_removing_current_thread == 1 && "Current thread was not removed from threadlist.");
  debug(SYSCALL, "Process has currently %ld thread in his list.\n", threads_of_process.size());
  currentThread->kill();
}

bool Syscall::check_parameter(size_t ptr, bool allowed_to_be_null)
{
    if(!allowed_to_be_null && ptr == 0)
    {
      return false;
    }
    debug(SYSCALL, "Ptr %p USER_BREAK %p.\n",(void*)ptr, (void*)USER_BREAK);
    if(ptr >= USER_BREAK)
    {
      return false;
    }
    return true;
}