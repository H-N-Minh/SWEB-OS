#pragma once

#include "Mutex.h"
#include "uvector.h"
#include "umap.h"
#include "ArchMemory.h"

class ArchMemory;

struct VirtualPageInfo
{
  size_t vpn_;
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
    void addVirtualPageInfo(size_t ppn, size_t vpn, ArchMemory* archmemory);
    void addVirtualPageInfos(size_t ppn, ustl::vector<VirtualPageInfo*> page_infos);
    ustl::vector<VirtualPageInfo*> getAndRemoveVirtualPageInfos(size_t ppn);

  private:
    static InvertedPageTable* instance_;
    ustl::map<size_t, ustl::vector<VirtualPageInfo*>> ipt_; //ppn - pageInfos(vpn...)

};