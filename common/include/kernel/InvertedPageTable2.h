#pragma once

#include "Mutex.h"
#include "uvector.h"
#include "umap.h"
#include "ArchMemory.h"
#include "InvertedPageTable.h"

class ArchMemory;



class InvertedPageTable2
{
  public:
    InvertedPageTable2();
    InvertedPageTable2(const InvertedPageTable2&) = delete;

    static InvertedPageTable2 *instance();
    
    Mutex ipt2_lock_;

    bool PPNisInMap(size_t ppn);
    void addPage(size_t ppn, size_t vpn, ArchMemory* archmemory);
    ustl::vector<VirtualPageInfo*> getAndRemoveVirtualPageInfos(size_t ppn);

  private:
    static InvertedPageTable2* instance_;
    ustl::map<size_t, ustl::vector<VirtualPageInfo*>> ipt2_; //offset in disk - pageInfos(vpn...)

};