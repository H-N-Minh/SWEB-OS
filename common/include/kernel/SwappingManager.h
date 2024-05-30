#pragma once

#include "BDVirtualDevice.h"
#include "BDManager.h"

#include "IPTManager.h"

class IPTEntry;
class SwappingManager
{
  public:
    static SwappingManager *instance();
    SwappingManager();
    SwappingManager(const SwappingManager&) = delete;

    ~SwappingManager();

    void swapOutPage(size_t ppn);
    int swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages);

    void directSwapOutPage(size_t ppn);
    int directSwapInPage(size_t vpn, ustl::vector<uint32>& preallocated_pages);

    void lock_archmemories_in_right_order(ustl::vector<IPTEntry*> &virtual_page_infos);
    void unlock_archmemories(ustl::vector<IPTEntry*> &virtual_page_infos);

    int getDiskWrites();
    int getDiskReads();

    // locking order: Ipt -> disk -> archmem
    Mutex disk_lock_;

  private:
    static SwappingManager* instance_;
    IPTManager* ipt_;

    BDVirtualDevice* bd_device_;

    static size_t disk_offset_counter_;    //TODO? this only goes up, so disk page is never reused

    int total_disk_reads_ = 0;
    int total_disk_writes_ = 0;
};