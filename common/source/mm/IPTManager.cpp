#include "IPTManager.h"
#include "debug.h"

void IPTManager::addEntryToRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::addEntryToRAM: Adding entry to RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
  ramMap.insert({ppn, ustl::make_shared<IPTEntry>(ppn, vpn, archmem)});
}

void IPTManager::addEntryToDisk(diskoffset_t diskOffset, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::addEntryToDisk: Adding entry to Disk: diskOffset=%d, vpn=%zu\n", diskOffset, vpn);
  diskMap[diskOffset] = ustl::make_shared<IPTEntry>(-1, vpn, archmem);
}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::lookupEntryInRAM: Looking up entry in RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
      debug(IPT, "IPTManager::lookupEntryInRAM: Entry found in RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
      return it->second;
    }
  }
  debug(IPT, "IPTManager::lookupEntryInRAM: Entry not found in RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
  return ustl::shared_ptr<IPTEntry>();
}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInDisk(diskoffset_t diskOffset) {
  debug(IPT, "IPTManager::lookupEntryInDisk: Looking up entry in Disk: diskOffset=%d\n", diskOffset);
  auto it = diskMap.find(diskOffset);
  if (it != diskMap.end()) {
    debug(IPT, "IPTManager::lookupEntryInDisk: Entry found in Disk: diskOffset=%d\n", diskOffset);
    return it->second;
  }
  debug(IPT, "IPTManager::lookupEntryInDisk: Entry not found in Disk: diskOffset=%d\n", diskOffset);
  return ustl::shared_ptr<IPTEntry>();
}

void IPTManager::removeEntryFromRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::removeEntryFromRAM: Removing entry from RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second;) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
      debug(IPT, "IPTManager::removeEntryFromRAM: Entry found and removed from RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
      it = ramMap.erase(it);
    } else {
      ++it;
    }
  }
}

void IPTManager::swapOutPage(ppn_t ppn, diskoffset_t diskOffset) {
  debug(IPT, "IPTManager::swapOutPage: Swapping out page: ppn=%d, diskOffset=%d\n", ppn, diskOffset);
  auto range = ramMap.equal_range(ppn);
  for (auto it = range.first; it != range.second;) {
    auto entry = it->second;
    it = ramMap.erase(it);
    diskMap[diskOffset] = entry;
    debug(IPT, "IPTManager::swapOutPage: Entry swapped out to Disk: ppn=%d, diskOffset=%d\n", ppn, diskOffset);
  }
}

void IPTManager::swapInPage(diskoffset_t diskOffset, ppn_t ppn) {
  debug(IPT, "IPTManager::swapInPage: Swapping in page: diskOffset=%d, ppn=%d\n", diskOffset, ppn);
  auto entry = diskMap[diskOffset];
  if (entry) {
    entry->ppn = ppn;
    diskMap.erase(diskOffset);
    ramMap.insert({ppn, entry});
    debug(IPT, "IPTManager::swapInPage: Entry swapped in from Disk: diskOffset=%d, ppn=%d\n", diskOffset, ppn);
  } else {
    debug(IPT, "IPTManager::swapInPage: Entry not found in Disk: diskOffset=%d\n", diskOffset);
  }
}
