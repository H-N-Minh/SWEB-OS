#pragma once

#include "Thread.h"
#include "Condition.h"
#include "Mutex.h"

class SwappingThread : public Thread
{
  public:
    SwappingThread();
    virtual ~SwappingThread();
    virtual void kill();
    virtual void Run();
  

    Mutex orders_lock_;
    Condition orders_cond_;
    int orders_;
};

