#pragma once

#include "Mutex.h"
#include "uvector.h"
#include "umap.h"
#include "ArchMemory.h"

struct VirtualPageInfo
{
  uint64 vpn_;
  ArchMemory* arch_memory_;
};


class InvertedPageTable
{
  public:
    InvertedPageTable();
    InvertedPageTable(const InvertedPageTable&) = delete;
    
    Mutex inverted_pagetable_lock_;

    bool PPNisInMap(size_t ppn);
    void addPage(uint64 ppn, uint64 vpn, ArchMemory* archmemory);

  private:
    ustl::map<uint64, ustl::vector<VirtualPageInfo*>> inverted_pagetable_; //ppn - pageInfos(vpn...)

};