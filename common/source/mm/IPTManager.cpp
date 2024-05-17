#include "IPTManager.h"

void IPTManager::addEntryToRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  ramMap.insert({ppn, ustl::make_shared<IPTEntry>(ppn, vpn, archmem)});
}

void IPTManager::addEntryToDisk(diskoffset_t diskOffset, size_t vpn, ArchMemory* archmem) {
  diskMap[diskOffset] = ustl::make_shared<IPTEntry>(-1, vpn, archmem);
}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
      return it->second;
    }
  }
  return ustl::shared_ptr<IPTEntry>();
}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInDisk(diskoffset_t diskOffset) {
  auto it = diskMap.find(diskOffset);
  return it != diskMap.end() ? it->second : ustl::shared_ptr<IPTEntry>();
}

void IPTManager::removeEntryFromRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
      ramMap.erase(it);
      break;
    }
  }
}

void IPTManager::swapOutPage(ppn_t ppn, diskoffset_t diskOffset) {
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    auto entry = it->second;
    ramMap.erase(it);
    diskMap[diskOffset] = entry;
  }
}

void IPTManager::swapInPage(diskoffset_t diskOffset, ppn_t ppn) {
  auto entry = diskMap[diskOffset];
  if (entry) {
    entry->ppn = ppn;
    diskMap.erase(diskOffset);
    ramMap.insert({ppn, entry});
  }
}
