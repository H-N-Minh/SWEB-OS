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
    static int user_initialized_flag_;    // 0: not initialized, 1: initialized. Only UserThread can change this flag.
    static bool should_be_killed_;

    SwappingThread();
    virtual ~SwappingThread();
    virtual void kill();
    virtual void Run();
  
    /////////////////// PRA metadata
  private:
    uint32 last_tick_;
  public:
    bool isOneTimeStep();
    void updateMetaData();


    ////////////////// PRA stats
  private:
    // both are protected by IPT lock
    uint32 hit_count_;    // the number of times a page is marked either dirty or accessed
    uint32 miss_count_;   // the number of times a page is swapped out
  public:
    uint32 getHitCount();
    uint32 getMissCount();



    /////////////// swap out
  private:
    ustl::vector<uint32> free_pages_;   // ppn that were swapped out and now free
  public:
    Mutex swap_out_lock_;     // swap_out_lock_ -> ipt_lock . protect the free_pages_
    Condition swap_out_cond_;

    /**
     * check if swap out is needed and performs it. Act as a main() func
    */
    void swapOut();

    /**
     * swap out a page based on the current PRA, then return the ppn of the page that is now free
    */
    size_t swapPageOut();

    /**
     * If the memory is almost full and we dont have enough free pages in free_pages_, we need to swap out some pages.
     * This satisfies both preswap and swap out on demand.
    */
    static bool isMemoryAlmostFull();
    static bool isMemoryFull();

    /**
     * check if free_pages_ is empty or not
    */
    bool isFreePageAvailable();

    /**
     * take a free page from free_pages_
    */
    uint32 getFreePage();


    //////////////// swap in
  private:
    ustl::multimap<size_t, ustl::vector<uint32>*> swap_in_map_; // <disk_offset, preallocated_pages>. Map of pages that needs to be swapped in
  public:
    Mutex swap_in_lock_;      // swap_in_lock_ -> ipt_lock . protect the swap_in_map_
    Condition swap_in_cond_;
    /**
     * check if swap in is needed and performs it. Act as a main() func
    */
    void swapIn();

    /**
     * add a disk page, that needs to be swapped in, to the swap_in_map_
    */
    void addSwapIn(size_t disk_offset, ustl::vector<uint32>* preallocated_pages);

    /**
     * check if the disk_offset is in the swap_in_map_. If not, that means the page has been swapped in.
    */
    bool isOffsetInMap(size_t disk_offset);
    static void preSwap();

};

