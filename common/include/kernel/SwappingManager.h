#pragma once

#include "BDVirtualDevice.h"
#include "BDManager.h"
#include "InvertedPageTable.h"

class SwappingManager
{
  public:
    static SwappingManager *instance();
    SwappingManager();
    SwappingManager(const SwappingManager&) = delete;

    void swapOutPage(size_t ppn);
    int swapInPage(size_t vpn);

    void lock_archmemories_in_right_order(ustl::vector<VirtualPageInfo*> virtual_page_infos);
    void unlock_archmemories(ustl::vector<VirtualPageInfo*> virtual_page_infos);

    Mutex disk_lock_;

  private:
    static SwappingManager* instance_;
    InvertedPageTable* ipt_;

    BDVirtualDevice* bd_device_;

    static int disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing;
};