#pragma once

#include "types.h"
#include "paging-definitions.h"
#include "SpinLock.h"
#include "Mutex.h"
#include "IPTEntry.h"
#include "umap.h"

#define DYNAMIC_KMM (0) // Please note that this means that the KMM depends on the page manager
// and you will have a harder time implementing swapping. Pros only!


enum IPTMapType {IPT_MAP, SWAPPED_PAGE_MAP, NONE};

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

    // COW
    ustl::map<uint32, uint32> page_reference_counts_;
    Mutex ref_count_lock_;

    void incrementReferenceCount(uint64 page_number);
    void decrementReferenceCount(uint64 page_number);
    uint32 getReferenceCount(uint64 page_number);

    // Inverted Page Table
    Mutex IPT_lock_;          // lock both inverted_page_table_ and swapped_page_map_
    ustl::map<uint64, IPTEntry*> inverted_page_table_;  // map<ppn, map<PTE*, archmem_lock> >
    ustl::map<uint64, IPTEntry*> swapped_page_map_;  // map<offset in disk, map<PTE*, archmem_lock> >

    /**
     * Insert the PTE to the entry (at key == ppn) of the Inverted Page Table or the Swapped Page Map.
     * This does not accept inserting the same pte twice to the same ppn entry. (asserts fail)
     * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
     * @param ppn The ppn to insert (or the offset in disk for SWAPPED_PAGE_MAP)
     * @param pte The PageTableEntry to insert
     * @param archmem_lock The lock of the archmem that the PTE belongs to
    */
    void insertEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte, Mutex* archmem_lock);

    /**
     * Remove a pte from an entry from the Inverted Page Table or the Swapped Page Map.
     * This does not accept removing a pte that does not exist at the specified entry. (asserts if this happens)
     * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
     * @param ppn The ppn to remove (or the offset in disk for SWAPPED_PAGE_MAP)
     * @param pte The PageTableEntry to remove
    */
    void removeEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte);

    /**
     * Remove an entry from the source map and insert it to the destination map.
     * @param source the map that the entry is in currently. Destination is then automatically the other map
     * @param ppn_source The location of the entry currently
     * @param ppn_destination Location of the new entry in the destination map. This new entry must be empty.
    */
    void moveEntry(IPTMapType source, uint64 ppn_source, uint64 ppn_destination);

    /**
     * based on the given address, check which map the entry is in currently.
     * @return IPTMapType that indicates which map currently has this ppn as key. 0 if not found in both.
    */
    IPTMapType isInWhichMap(uint64 ppn);
    

 
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
};
