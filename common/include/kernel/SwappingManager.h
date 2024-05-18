#pragma once

#include "InvertedPageTable2.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"

class SwappingManager
{
  public:
    static SwappingManager *instance();
    SwappingManager();
    SwappingManager(const SwappingManager&) = delete;

    void swapOutPage(size_t ppn);
    int swapInPage(size_t vpn);

  private:
    static SwappingManager* instance_;
    InvertedPageTable* ipt_;
    InvertedPageTable2* ipt2_;

    BDVirtualDevice* bd_device_;

    static int disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing;
};