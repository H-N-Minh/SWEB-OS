#pragma once

#include "Thread.h"
#include "Condition.h"
#include "Mutex.h"
#include "uvector.h"
#include "types.h"
#include "IPTManager.h"

class SwappingThread : public Thread
{
  public:
    SwappingThread();
    virtual ~SwappingThread();
    virtual void kill();
    virtual void Run();
  
    void swapPageOut();
    void swap10PagesOut();

    Mutex orders_lock_;
    Condition orders_cond_;
    int orders_;
    ustl::vector<uint32> free_pages_;

    static int ipt_initialized_flag_;    // 0: not initialized, 1: initialized. Only IPTManager can change this flag.

    bool isOneTimeStep();
    void updateMetaData();

    uint32 getHitCount();
    uint32 getMissCount();

  private:
    uint32 last_clock_;   // protected by orders_lock_

    uint32 hit_count_;    // protected by IPT lock
    uint32 miss_count_;   // protected by IPT lock
};

