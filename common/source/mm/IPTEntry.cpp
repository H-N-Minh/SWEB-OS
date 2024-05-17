#include "IPTEntry.h"

void IPTEntry::addEntry(PageTableEntry* pte, Mutex* lock) {
    entry_map_[pte] = lock;
}

void IPTEntry::removeEntry(PageTableEntry* pte) {
    entry_map_.erase(pte);
}

bool IPTEntry::isEmpty() {
    return entry_map_.empty();
}