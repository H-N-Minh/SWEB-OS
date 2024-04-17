#pragma once

// #include "new.h"
// #include "SpinLock.h"
#include "assert.h"
#include "types.h"
#include "Loader.h"
#include "SpinLock.h"


#define MAX_HEAP_SIZE (USER_BREAK / 4)


class UserSpaceMemoryManager
{
  public:
    UserSpaceMemoryManager(Loader* loader);

    size_t current_break_;
    size_t heap_start_;
    Loader* loader_;

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
     * increase the stack size by one page
     * @param address the address to increase the stack size at
     * @return 0 on success, else return -1
    */
    int increaseStackSize(size_t address);

    /**
     * @return the top of the current page. It cant be 0
    */
    size_t getTopOfThisPage();

}
