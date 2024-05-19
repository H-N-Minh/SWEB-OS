#pragma once

#include "Mutex.h"
#include "uvector.h"
#include "umap.h"

class ArchMemory;

struct VirtualPageInfo
{
  size_t vpn_;
  ArchMemory* arch_memory_;
};

enum MAPTYPE {IPT_RAM, IPT_DISK};

class InvertedPageTable
{
  public:
    InvertedPageTable();
    InvertedPageTable(const InvertedPageTable&) = delete;

    static InvertedPageTable *instance();
    
    Mutex ipt_lock_;

    bool KeyisInMap(size_t ppn, MAPTYPE map_type);
    void addVirtualPageInfo(size_t ppn, size_t vpn, ArchMemory* archmemory);

    ustl::vector<VirtualPageInfo*> getPageInfosForPPN(size_t ppn);

     ustl::map<size_t, ustl::vector<VirtualPageInfo*>>* selectMap(MAPTYPE map_type);
     ustl::vector<VirtualPageInfo*> moveToOtherMap(size_t old_key, size_t new_key, MAPTYPE from, MAPTYPE to);

  private:
    static InvertedPageTable* instance_;
    ustl::map<size_t, ustl::vector<VirtualPageInfo*>> ipt_ram_;
    ustl::map<size_t, ustl::vector<VirtualPageInfo*>> ipt_disk_; //ppn - pageInfos(vpn...)

};