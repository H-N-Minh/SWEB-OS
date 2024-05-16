#pragma once

#include "InvertedPageTable.h" 
#include "Mutex.h"
#include "uvector.h"
#include "umap.h"
#include "ArchMemory.h"



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
};