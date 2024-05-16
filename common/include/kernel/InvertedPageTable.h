#pragma once

#include "Mutex.h"
#include "uvector.h"
#include "umap.h"
#include "ArchMemory.h"

class ArchMemory;

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

    static InvertedPageTable *instance();
    
    Mutex ipt_lock_;

    bool PPNisInMap(size_t ppn);
    void addPage(uint64 ppn, uint64 vpn, ArchMemory* archmemory);

  private:
    static InvertedPageTable* instance_;
    ustl::map<uint64, ustl::vector<VirtualPageInfo*>> ipt_; //ppn - pageInfos(vpn...)

};