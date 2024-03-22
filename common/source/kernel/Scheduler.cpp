#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include <ulist.h>
#include "backtrace.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "umap.h"
#include "ustring.h"
#include "Lock.h"
#include "ProcessRegistry.h"

ArchThreadRegisters *currentThreadRegisters;
Thread *currentThread;

Scheduler *Scheduler::instance_ = nullptr;

Scheduler *Scheduler::instance()
{
  if (unlikely(!instance_))
    instance_ = new Scheduler();
  return instance_;
}

Scheduler::Scheduler()
{
  block_scheduling_ = 0;
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  addNewThread(&idle_thread_);
}

void Scheduler::schedule()
{
  assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");
  if (block_scheduling_)
  {
    debug(SCHEDULER, "schedule: currently blocked\n");
    return;
  }

  auto it = threads_.begin();
  for(; it != threads_.end(); ++it)
  {
    if((*it)->schedulable())
    {
      currentThread = *it;
      break;
    }
  }

//    for(; it != threads_.end(); ++it)
//    {
//        if((*it)->schedulable())
//        {
//            UserThread& currentUserThread = *((UserThread*)*it);
//            if(currentUserThread.can_be_canceled_ && currentUserThread.switch_to_userspace_ && currentUserThread.cancel_type_ != PTHREAD_CANCEL_DEFERRED)
//            {
//                if(currentUserThread.cancel_type_ == PTHREAD_CANCEL_ASYNCHRONOUS && currentUserThread.cancel_state_ == PTHREAD_CANCEL_DISABLE)
//                    break;
//
//                //call pthread exit here?
//
//                currentUserThread.switch_to_userspace_ = 0;
//            }
//        }

  assert(it != threads_.end() && "No schedulable thread found");
  ustl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled
  //debug(SCHEDULER, "Scheduler::schedule: new currentThread is %p %s, switch_to_userspace: %d\n", currentThread, currentThread->getName(), currentThread->switch_to_userspace_);
  currentThreadRegisters = currentThread->switch_to_userspace_ ? currentThread->user_registers_ : currentThread->kernel_registers_;
}

void Scheduler::addNewThread(Thread *thread)
{
  assert(thread);
  debug(SCHEDULER, "addNewThread: %p  %zd:%s\n", thread, thread->getTID(), thread->getName());
  if (currentThread)
    ArchThreads::debugCheckNewThread(thread);
  KernelMemoryManager::instance()->getKMMLock().acquire();
  lockScheduling();
  KernelMemoryManager::instance()->getKMMLock().release();
  threads_.push_back(thread);
  unlockScheduling();
}

void Scheduler::sleep()
{
  currentThread->setState(Sleeping);
  assert(block_scheduling_ == 0);
  yield();
}

void Scheduler::wake(Thread* thread_to_wake)
{
  // wait until the thread is sleeping
  while(thread_to_wake->getState() != Sleeping)
    yield();
  thread_to_wake->setState(Running);
}

void Scheduler::yield()
{
  assert(this);
  if (!ArchInterrupts::testIFSet())
  {
    assert(currentThread);
    kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %p %s)\n",
             currentThread, currentThread->name_.c_str());
    currentThread->printBacktrace();
  }
  ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
  /* Before adding new functionality to this function, consider if that
     functionality could be implemented more cleanly in another place.
     (e.g. Thread/Process destructor) */

  assert(currentThread == &cleanup_thread_);

  lockScheduling();
  uint32 thread_count_max = sizeof(cleanup_thread_.kernel_stack_) / (2 * sizeof(Thread*));
  thread_count_max = ustl::min(thread_count_max, threads_.size());
  Thread* destroy_list[thread_count_max];
  uint32 thread_count = 0;
  for (uint32 i = 0; i < threads_.size() && thread_count < thread_count_max; ++i)
  {
    Thread* tmp = threads_[i];
    if (tmp->getState() == ToBeDestroyed)
    {
      destroy_list[thread_count++] = tmp;
      threads_.erase(threads_.begin() + i); // Note: erase will not realloc!
      --i;
    }
  }
  unlockScheduling();
  if (thread_count > 0)
  {
    for (uint32 i = 0; i < thread_count; ++i)
    {
      delete destroy_list[i];
    }
    debug(SCHEDULER, "cleanupDeadThreads: done\n");
  }
}

// void Scheduler::cleanupDeadProcesses()                        //123
// {
//   lockScheduling();                                      //not sure if this is right
//   UserProcess* destroy_list[500];                             //!! should be max number of processes
//   int destroy_index[500];                             //!! should be max number of processes
//   int processes_to_delete = 0;
//   int index = 0;
//   for(auto& process: ProcessRegistry::instance()->processes_)
//   {
//     if(process->to_be_destroyed_)
//     {
//       destroy_list[processes_to_delete] = process;
//       destroy_index[processes_to_delete] = index;
//       processes_to_delete++;
//     }
//     index++;
//   }
//   if (processes_to_delete > 0)
//   {
//     for (int i = 0; i < processes_to_delete; ++i)
//     {
//       delete destroy_list[i];
//     }

//     int deleted_values = 0;
//     for (auto& index : destroy_index)
//     {
//       ProcessRegistry::instance()->processes_.erase(ProcessRegistry::instance()->processes_.begin()+index-deleted_values);
//       deleted_values++;
//       if(deleted_values >= processes_to_delete)
//       {
//         break;
//       }
//     }
//   }
//   unlockScheduling();            //not sure if this is right
// }

void Scheduler::printThreadList()
{
  lockScheduling();
  debug(SCHEDULER, "Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
  for (size_t c = 0; c < threads_.size(); ++c)
    debug(SCHEDULER, "Scheduler::printThreadList: threads_[%zd]: %p  %zd:%25s     [%s]\n", c, threads_[c],
          threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_]);
  unlockScheduling();
}

void Scheduler::lockScheduling() //not as severe as stopping Interrupts
{
  if (unlikely(ArchThreads::testSetLock(block_scheduling_, 1)))
    kpanict("FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n");
}

void Scheduler::unlockScheduling()
{
  block_scheduling_ = 0;
}

bool Scheduler::isSchedulingEnabled()
{
  return this && block_scheduling_ == 0;
}

bool Scheduler::isCurrentlyCleaningUp()
{
  return currentThread == &cleanup_thread_;
}

size_t Scheduler::getTicks()
{
  return ticks_;
}

void Scheduler::incTicks()
{
  ++ticks_;
}

void Scheduler::printStackTraces()
{
  lockScheduling();
  debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

  for (const auto& thread : threads_)
  {
    thread->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  unlockScheduling();
}

void Scheduler::printLockingInformation()
{
  lockScheduling();
  kprintfd("\n");
  debug(LOCK, "Scheduler::printLockingInformation:\n");

  for(Thread* t : threads_)
  {
    if(t->holding_lock_list_)
      Lock::printHoldingList(t);
  }
  for(Thread* t : threads_)
  {
    if(t->lock_waiting_on_)
      debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p).\n",
            t->getName(), t, t->lock_waiting_on_ ->getName(), t->lock_waiting_on_ );
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  unlockScheduling();
}

uint32 Scheduler::getThreadCount() {
    return threads_.size();
}