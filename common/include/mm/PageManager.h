#pragma once

#include "types.h"
#include "paging-definitions.h"
#include "SpinLock.h"
#include "Mutex.h"

#include "umap.h"

#define DYNAMIC_KMM (0) // Please note that this means that the KMM depends on the page manager
// and you will have a harder time implementing swapping. Pros only!

class Bitmap;

class PageManager
{
  public:
    static PageManager *instance();

    /**
     * Returns the total number of physical pages available to SWEB.
     * @return Number of available physical pages
     */
    uint32 getTotalNumPages() const;

    /**
     * Returns the number of currently free physical pages.
     * @return Number of free physical pages
     */
    uint32 getNumFreePages() const;

    /**
     * Marks the lowest free physical page as used and returns it.
     * Always returns 4KB PPNs.
     * @param page_size The requested page size, must be a multiple of PAGE_SIZE
     * @return The allocated physical page PPN.
     */
    uint32 allocPPN(uint32 page_size = PAGE_SIZE);

    /**
     * Marks the given physical page as free
     * @param page_number The physical page PPN to free
     * @param page_size The page size to free, must be a multiple of PAGE_SIZE
     */
    void freePPN(uint32 page_number, uint32 page_size = PAGE_SIZE);

    Thread* heldBy()
    {
      return page_manager_lock_.heldBy();
    }

    PageManager();

    void printBitmap();

    uint32 getNumPagesForUser() const;

//    struct PageInfo{
//        uint32 reference_count;
//    };
    // <ppn, count>
    ustl::map<uint32, uint32> page_reference_counts_;
    Mutex page_reference_counts_lock_;

    void incrementReferenceCount(uint64 page_number);
    void decrementReferenceCount(uint64 page_number);

    uint32 getReferenceCount(uint64 page_number);

  private:
    bool reservePages(uint32 ppn, uint32 num = 1);

    PageManager(PageManager const&);

    Bitmap* page_usage_table_;
    uint32 number_of_pages_;
    uint32 lowest_unreserved_page_;
    uint32 num_pages_for_user_;

    SpinLock page_manager_lock_;

    static PageManager* instance_;

    size_t HEAP_PAGES;

    inline static int delete_later_counter = 1025;

    size_t findPageToSwapOut();
};
