#pragma once

#include "types.h"
#include "uvector.h"

#define PRESENT 3
#define INVALID 0
#define VALID 1
#define GROWING_STACK_VALID 69
#define USER 5
#define ERROR 11
#define NOT_RELATED_TO_GROWING_STACK 17
#define GROWING_STACK_FAILED 18


#define DIRECT_SWAPPING 0
#define ASYNCHRONOUS_SWAPPING 1

class PageFaultHandler
{
private:
  /**
   * The border address at which it is assumed that
   * a pagefault happened by dereferencing a null pointer.
   */
  static const size_t null_reference_check_border_;

  /**
   * Print out the pagefault information. Check if the pagefault is valid, or the thread state is corrupt.
   * Afterwards, load a the if necessary.
   * @param address The address on which the fault happened
   * @param user true if the fault occurred in user mode, else from kernel mode
   * @param present true if the fault happened on a already mapped page
   * @param switch_to_us the switch to userspace flag of the current thread
   * @return 1 if valid, 0 if invalid or segmentation fault, 69 if its growing stack
   */
  static inline int checkPageFaultIsValid(size_t address, bool user, bool present, bool switch_to_us);

  /**
   * Print out the pagefault information. Check if the pagefault is valid, or the thread state is corrupt.
   * Afterwards, load a the if necessary.
   * @param address The address on which the fault happened
   * @param user true if the fault occurred in user mode, else from kernel mode
   * @param present true if the fault happened on a already mapped page
   * @param writing true if the fault happened by writing to an address, else reading
   * @param fetch true in case the fault happened by an instruction fetch, else by an operand fetch
   * @param switch_to_us the switch to userspace flag of the current thread
   */
  static inline void handlePageFault(size_t address, bool user,
                                     bool present, bool writing,
                                     bool fetch, bool switch_to_us);

public:
  /**
   * Enter a new pagefault. The pagefault is processed.
   * Afterwards, a context switch is performed (if needed).
   * @param address The address on which the fault happened
   * @param user true if the fault occurred in user mode, else from kernel mode
   * @param present true if the fault happened on a already mapped page
   * @param writing true if the fault happened by writing to an address, else reading
   * @param fetch true in case the fault happened by an instruction fetch, else by an operand fetch
   */
  static void enterPageFault(size_t address, bool user,
                             bool present, bool writing,
                             bool fetch);

  static int checkGrowingStack(size_t address);
  static void errorInPageFaultKillProcess();

};
