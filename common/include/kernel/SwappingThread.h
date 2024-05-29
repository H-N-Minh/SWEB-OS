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

    Mutex orders_lock_;
    Condition orders_cond_;
    int orders_;
    ustl::vector<uint32> free_pages_;

    static int ipt_initialized_flag_;    // 0: not initialized, 1: initialized. Only IPTManager can change this flag.

    bool isOneTimeStep();
    void updateMetaData();
  
  private:
    uint32 last_clock_;
};

