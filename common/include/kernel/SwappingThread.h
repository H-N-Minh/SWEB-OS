#pragma once

#include "Thread.h"
#include "Condition.h"
#include "Mutex.h"
#include "uvector.h"
#include "types.h"

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
};

