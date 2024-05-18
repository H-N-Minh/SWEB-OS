
#pragma once

#include "Mutex.h"
#include "umap.h"
#include "paging-definitions.h"


class IPTEntry {

private:
    // map: <pointer to PTE , pointer to the lock needed before modifying the PTE>
    ustl::map<PageTableEntry*, Mutex*> entry_map_;      // keep this private, use methods instead
    // friend class PageManager;
public:

    IPTEntry() {}
    
    void addEntry(PageTableEntry* pte, Mutex* lock);

    void removeEntry(PageTableEntry* pte);

    bool isEmpty();

    /**
     * check if the archmem of the specified PTE is locked by currentThread or not
    */
    bool isLocked(PageTableEntry* pte);
};