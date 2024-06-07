#include "IPTEntry.h"
#include "ArchMemory.h"
#include "ArchThreads.h"


////////////////////// ArchmemIPT //////////////////////

ArchmemIPT::ArchmemIPT(size_t vpn, ArchMemory* archmem) : vpn_(vpn), archmem_(archmem) {}

bool ArchmemIPT::isLockedByUs() const
{
  return archmem_->archmemory_lock_.isHeldBy((Thread*) currentThread);
}

////////////////////// IPTEntry //////////////////////

IPTEntry::IPTEntry() : pre_swapped_(false), disk_offset_(0), access_counter_(0) {}


IPTEntry::~IPTEntry()
{
  for (auto& archmemIPT : archmemIPTs_)
  {
    delete archmemIPT;
  }
}


bool IPTEntry::isArchmemExist(ArchMemory* searchArchmem) {
  auto archmemIPTs = getArchmemIPTs();
  for (auto& currentArchmemIPT : archmemIPTs)
  {
    bool isFound = currentArchmemIPT->archmem_ == searchArchmem;
    if (isFound) {
      return true;
    }
  }
  return false;
}


bool IPTEntry::isEmpty()
{
  return archmemIPTs_.empty();
}

void IPTEntry::addArchmemIPT(size_t vpn, ArchMemory* archmem)
{
  auto* newArchmemIPT = new ArchmemIPT(vpn, archmem);
  archmemIPTs_.push_back(newArchmemIPT);
}

void IPTEntry::removeArchmemIPT(size_t vpn, ArchMemory* archmem)
{
  for (auto it = archmemIPTs_.begin(); it != archmemIPTs_.end(); ++it)
  {
    if ((*it)->vpn_ == vpn && (*it)->archmem_ == archmem)
    {
      delete *it;
      archmemIPTs_.erase(it);
      return;
    }
  }
}

ustl::vector<ArchmemIPT*>& IPTEntry::getArchmemIPTs()
{
  return archmemIPTs_;
}

void IPTEntry::setPreSwapped(size_t disk_offset)
{
  pre_swapped_ = true;
  disk_offset_ = disk_offset;
}

bool IPTEntry::isPreSwapped() const
{
  return pre_swapped_;
}

size_t IPTEntry::getDiskOffset() const
{
  return disk_offset_;
}