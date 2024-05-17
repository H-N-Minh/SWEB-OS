#pragma once

// #include "new.h"
// #include "SpinLock.h"
#include "assert.h"
#include "types.h"
#include "Loader.h"
#include "SpinLock.h"
#include "UserThread.h"
#include "Scheduler.h"
#include "IPTManager.h"


#define MAX_HEAP_SIZE (USER_BREAK / 4)
#define MAX_STACK_AMOUNT 5    // if this is changed then update the define in pthread.h in userspace 
#define GUARD_MARKER 0xbadcafe00000ULL  

class UserSpaceMemoryManager
{
  public:
    UserSpaceMemoryManager(Loader* loader);

    size_t current_break_;
    size_t heap_start_;
    Loader* loader_;
    IPTManager iptManager;
    SpinLock lock_;   // used to protect current_break_

    size_t totalUsedHeap();

    /**
     * adjust the brk by size amount 
     * @param size the amount to adjust the brk by (can be positive or negative)
     * @param already_locked if the lock for break is already held
     * @return pointer to the reserved space, else return 0 on failure
    */
    pointer sbrk(ssize_t size, size_t already_locked);

    /**
     * set the address of brk to a fixed address
     * @return 0 on success, else return -1
    */
    int brk(size_t new_break_addr);

    /**
     * check if the address is a valid growing stack address
     * @param address the address to check
     * @return 1 if the address is valid, 11 if its a segment fault, else return 0.
    */
    int checkValidGrowingStack(size_t address);


    /**
     * allocate a new page and map to the page with the given address
     * @param address the address to increase the stack size at
     * @return 0 on success, else return -1
    */
    int increaseStackSize(size_t address);

    /**
     * @return the top of the page of the given address. 0 is never returned
    */
    size_t getTopOfThisPage(size_t address) ;

    /** check for overflow/underflow corruption. If Guards are still intact, then return the top of the stack
     * @return 0 if guard is corrupted or not found, 1 if guard is still intact
    */
    int checkGuardValid(size_t top_current_stack);

    /**
     * Final check to make sure growing stack is valid
    */
    void finalSanityCheck(size_t address, size_t top_current_stack);

    /**
     * first check to quickly see if the address is somewhat valid
     * @return 1 if the address seems valid, 0 if address seems not related to growing stack
    */
    int sanityCheck(size_t address);

    /**
     * get the top of the stack of the given address
     * @return the top of the stack, 0 if not found
    */
    size_t getTopOfThisStack(size_t address);

};
