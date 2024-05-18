#pragma once

#include "umap.h"
#include "Mutex.h"

struct InvertedPageTableEntry {
    uint64_t virtual_page_;
    uint64_t pid_;
};

class InvertedPageTable {
private:
    ustl::map<uint64_t, InvertedPageTableEntry> invertedPageTable;
public:
    void addEntry(uint64_t physical_page, uint64_t virtual_page, uint64_t pid);
    void removeEntry(uint64_t physical_page);

    InvertedPageTableEntry getEntry(uint64_t physical_page);

};