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
    size_t preSwapPageToDisk(const char* page_content);
    void handlePreSwap();

    static void lock_archmemories_in_right_order(ustl::vector<ArchmemIPT*> &virtual_page_infos);
    static void unlock_archmemories(ustl::vector<ArchmemIPT*> &virtual_page_infos);

    int getDiskWrites() const;
    int getDiskReads() const;

    // locking order: Ipt -> disk -> archmem
    Mutex disk_lock_;

  private:
    static SwappingManager* instance_;
    IPTManager* ipt_;
    BDVirtualDevice* bd_device_;

    ustl::queue<size_t> pre_swap_queue_;
    Mutex pre_swap_lock_;
    static size_t disk_offset_counter_;    //TODO? this only goes up, so disk page is never reused

    int total_disk_reads_ = 0;
    int total_disk_writes_ = 0;
};