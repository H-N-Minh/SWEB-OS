#include "IPTEntry.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"

void IPTEntry::addEntry(PageTableEntry* pte, Mutex* lock) {
    if (entry_map_.find(pte) != entry_map_.end()) {
        debug(SWAPPING, "IPTEntry::addEntry PageTableEntry* %lu already exists in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
        assert(0 && "IPTEntry::addEntry PageTableEntry* already exists in the IPTEntry \n");
    } else {
        entry_map_[pte] = lock;
    }
}

void IPTEntry::removeEntry(PageTableEntry* pte) {
    if (entry_map_.find(pte) == entry_map_.end()) {
        debug(SWAPPING, "IPTEntry::removeEntry PageTableEntry* %lu not found in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
        assert(0 && "IPTEntry::removeEntry: Cant remove PageTableEntry* that doesnt exist in IPTEntry\n");
    } else {
        entry_map_.erase(pte);
    }
}

bool IPTEntry::isLocked(PageTableEntry* pte) {
    if (entry_map_.find(pte) == entry_map_.end()) {
        debug(SWAPPING, "IPTEntry::removeEntry PageTableEntry* %lu not found in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
        assert(0 && "IPTEntry::removeEntry: Cant remove PageTableEntry* that doesnt exist in IPTEntry\n");
        return false;
    } else {
        return entry_map_[pte]->isHeldBy((Thread*) currentThread);
    }
}

bool IPTEntry::isEmpty() {
    return entry_map_.empty();
}