#pragma once

#include "BDVirtualDevice.h"
#include "BDManager.h"

#include "IPTManager.h"
#include "uqueue.h"

class ArchmemIPT;
class SwappingManager
{
  public:
    static SwappingManager *instance();
    SwappingManager();
    SwappingManager(const SwappingManager&) = delete;

    ~SwappingManager();

    void swapOutPage(size_t ppn);
    int swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages);

    static void lock_archmemories_in_right_order(ustl::vector<ArchmemIPT*> &virtual_page_infos);
    static void unlock_archmemories(ustl::vector<ArchmemIPT*> &virtual_page_infos);

    int getDiskWrites() const;
    int getDiskReads() const;

    // locking order: Ipt -> disk -> archmem
    Mutex disk_lock_;

    bool preSwapPage(size_t ppn);

    static bool pre_swap_enabled;

  private:
    static SwappingManager* instance_;
    IPTManager* ipt_;
    BDVirtualDevice* bd_device_;

    static size_t disk_offset_counter_;    //TODO? this only goes up, so disk page is never reused

    int total_disk_reads_ = 0;
    int total_disk_writes_ = 0;
};