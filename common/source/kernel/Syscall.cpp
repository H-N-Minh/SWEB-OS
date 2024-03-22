#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "Scheduler.h"

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
      break;

    case sc_pthread_cancel:
        return_value = cancelThread((size_t)arg1);
        break;

      case sc_pthread_setcancelstate:
          return_value = pthread_setcancelstate((int)arg1, (int *)arg2);
          break;
      case sc_pthread_setcanceltype:
          return_value = pthread_setcanceltype((int)arg1, (int *)arg2);
          break;
      case sc_pthread_exit:
          return_value = exitThread((void*) arg1);
          break;

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

uint32 Syscall::exitThread(void* return_value)
{
    debug(TAI_THREAD, "Syscall::exitThread: zombie the current thread \n");
    ((UserThread*) currentThread)->setReturnValue(return_value);
    Scheduler::instance()->sleep();
    return 0;
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
            // after thread kill itself, it is removed from Vector, so we need to decrement i and vector size
            i--;
            vector_size--;
        }
    }

    Scheduler::instance()->printThreadList();
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

int Syscall::createThread(void* func, void* para, void* tid, void* pcreate_helper)
{

    //debug(TAI_THREAD, "------------------------------------thread %p, attribute %p, func %p args %p\n", thread, attr, start_routine, arg);
    Scheduler::instance()->printThreadList();
    ((UserThread*) currentThread)->process_->createThread(func, para, tid, pcreate_helper);
    Scheduler::instance()->printThreadList();
    return 0;
}

int Syscall::cancelThread(size_t thread_id)
{

    debug(TAI_THREAD, "--------------------Cancelling thread %zu\n", thread_id);
    UserThread* canceled_thread = ((UserThread*)currentThread)->process_->getUserThread(thread_id);

    debug(TAI_THREAD, "--------------------canceled_thread %p\n", canceled_thread);

    if (!canceled_thread)
    {
        debug(TAI_THREAD, "------------------Syscall::pthread_cancel: Thread %zu not found\n", thread_id);
        return -1; //hread not found
    }

    //cancel the thread
    canceled_thread->can_be_canceled_ = true;
    //debug(TAI_THREAD, "--------------Can be cancelled value after %d\n", canceled_thread->can_be_canceled_);
    return 0; //successful cancellation


}

int Syscall::pthread_setcancelstate(int state, int *oldstate)
{
    if(state != 0 && state != 1) //not enable or disable in userspace
    {
        debug(TAI_THREAD, "------------------Call cancel state fail\n");
        return -1;
    }

    CancelState previous_state = ((UserThread*) currentThread)->getCancelState(); //the state the its currently have
    *oldstate = (int)previous_state;

    ((UserThread*) currentThread)->setCancelState((CancelState)state);

    debug(TAI_THREAD, "----------------current state %s, previous state %s\n",
          state == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED",
          *oldstate == CancelState::PTHREAD_CANCEL_ENABLE ? "ENABLED" : "DISABLED");

    return 0; //success
}

int Syscall::pthread_setcanceltype(int type, int *oldtype)
{
    if(type != 3 && type != 4) //not ASYNCHRONOUS or DEFERRED in userspace
    {
        debug(TAI_THREAD, "------------------Call cancel type fail\n");
        return -1;
    }

    CancelType previous_type = ((UserThread*) currentThread)->getCancelType(); //the type the its currently have
    *oldtype = (int)previous_type;

    ((UserThread*) currentThread)->setCancelType((CancelType)type);

    debug(TAI_THREAD, "------------------current type %s, previous type %s\n",
          type == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS",
          *oldtype == CancelType::PTHREAD_CANCEL_DEFERRED ? "DEFERRED" : "ASYNCHRONOUS");

    return 0; //success


    return 0; //success
}