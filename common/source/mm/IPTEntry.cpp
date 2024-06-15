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

IPTEntry::IPTEntry()
{
  access_counter_ = 0;
}

IPTEntry::~IPTEntry()
{
  for (auto& archmemIPT : archmemIPTs_)
  {
    delete archmemIPT;
  }
  access_counter_ = 0;
}


bool IPTEntry::isArchmemExist(ArchMemory* archmem, size_t vpn)
{
    for (auto& archmemIPT : archmemIPTs_)
    {
        if (archmemIPT->archmem_ == archmem && archmemIPT->vpn_ == vpn)
        {
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