#pragma once

#include "BDVirtualDevice.h"
#include "BDManager.h"

#include "IPTManager.h"
#include "uqueue.h"

#include "Condition.h"

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

    void lock_archmemories_in_right_order(ustl::vector<ArchmemIPT*> &virtual_page_infos);  //TODOs //maybe use this
    void unlock_archmemories(ustl::vector<ArchmemIPT*> &virtual_page_infos);

    int getDiskWrites() const;
    int getDiskReads() const;

    // locking order: Ipt -> disk -> archmem
    Mutex disk_lock_;

    bool preSwapPage(size_t ppn);

    static bool pre_swap_enabled;
    Mutex swapping_thread_finished_lock_;
    Condition swapping_thread_finished_;

  private:
    static SwappingManager* instance_;
    IPTManager* ipt_;
    BDVirtualDevice* bd_device_;


    static size_t disk_offset_counter_;    //TODO? this only goes up, so disk page is never reused


    int total_disk_reads_ = 0;
    int total_disk_writes_ = 0;
    ustl::vector<size_t> pre_swapped_pages;


    void lockArchmemorys(ustl::vector<ArchmemIPT*>& virtual_page_infos);
    void unlockArchmemorys(ustl::vector<ArchmemIPT*>& virtual_page_infos);

    void printDebugInfos(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn, size_t disk_offset);

    void writeToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset);

    void updatePageTableEntriesForSwapOut(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset);
    void updatePageTableEntriesForSwapIn(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn, size_t disk_offset);

    void readFromDisk(size_t disk_offset, size_t ppn);

    bool isPageUnchanged(ustl::vector<ArchmemIPT*> &virtual_page_infos);

    void updatePageTableEntriesForWriteBackToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos);
};