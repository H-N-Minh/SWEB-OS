#include "IPTManager.h"
#include "debug.h"


#include "IPTEntry.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"

void IPTEntry::addEntry(PageTableEntry* pte, Mutex* lock) {
  if (entry_map_.find(pte) != entry_map_.end()) {
    debug(SWAPPING, "IPTEntry::addEntry PageTableEntry* %lu already exists in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
    assert(0 && "IPTEntry::addEntry PageTableEntry* already exists in the IPTEntry \n");
  } else {
    entry_map_[pte] = lock;
  }
}

void IPTEntry::removeEntry(PageTableEntry* pte) {
  if (entry_map_.find(pte) == entry_map_.end()) {
    debug(SWAPPING, "IPTEntry::removeEntry PageTableEntry* %lu not found in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
    assert(0 && "IPTEntry::removeEntry: Cant remove PageTableEntry* that doesnt exist in IPTEntry\n");
  } else {
    entry_map_.erase(pte);
  }
}

bool IPTEntry::isLocked(PageTableEntry* pte) {
  if (entry_map_.find(pte) == entry_map_.end()) {
    debug(SWAPPING, "IPTEntry::removeEntry PageTableEntry* %lu not found in the IPTEntry %lu\n", (size_t) pte, (size_t) this);
    assert(0 && "IPTEntry::removeEntry: Cant remove PageTableEntry* that doesnt exist in IPTEntry\n");
    return false;
  } else {
    return entry_map_[pte]->isHeldBy((Thread*) currentThread);
  }
}

bool IPTEntry::isEmpty() {
  return entry_map_.empty();
}




//IPT_lock_("PageManager::IPT_lock_")
//-----------------------------------------------------------------------------------------------------------------------------
ustl::shared_ptr<IPTEntry> IPTManager::createIPTEntry(int ppn, size_t vpn, ArchMemory* archmem) {
  return ustl::make_shared<IPTEntry>(ppn, vpn, archmem);
}

void IPTManager::addEntryToRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::addEntryToRAM: Adding entry to RAM: ppn=%d, vpn=%zu\n", ppn, vpn);
  ramMap.insert({ppn, createIPTEntry(ppn, vpn, archmem)});
}

void IPTManager::addEntryToDisk(diskoffset_t diskOffset, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::addEntryToDisk: Adding entry to Disk: diskOffset=%d, vpn=%zu\n", diskOffset, vpn);
  diskMap[diskOffset] = createIPTEntry(-1, vpn, archmem);
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





void IPTManager::insertEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte, Mutex* archmem_lock)
{
  debug(SWAPPING, "PageManager::insertEntryIPT: inserting ppn: %zu, pte: %zu, lock: %zu\n", ppn, (size_t) pte, (size_t) archmem_lock);
  assert(map_type == IPTMapType::NONE && pte && archmem_lock && "PageManager::insertEntryIPT called with a nullptr argument\n");
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem_lock->isHeldBy((Thread*) currentThread) && "PageManager::insertEntryIPT called without fully locking\n");

  auto* map = reinterpret_cast<ustl::map<uint64, IPTEntry *> *>(map_type == IPTMapType::IPT_MAP
                                                                                        ? &inverted_page_table_
                                                                                        : &swapped_page_map_);




  if (map->find(ppn) == map->end())
  {
    (*map)[ppn] = new IPTEntry();
  }
  (*map)[ppn]->addEntry(pte, archmem_lock);

  debug(SWAPPING, "PageManager::insertIPT: successfully inserted to IPT\n");
}

void IPTManager::removeEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte)
{
  // Error checking
  debug(SWAPPING, "PageManager::removeEntryIPT: removing ppn: %zx, pte: %zx\n", ppn, (size_t) pte);
  assert(map_type == IPTMapType::NONE && pte && "PageManager::removeEntryIPT called with a nullptr argument\n");
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "PageManager::insertEntryIPT called but IPT not locked\n");

  ustl::map<uint64, IPTEntry*>* map = (map_type == IPTMapType::IPT_MAP ? &inverted_page_table_ : &swapped_page_map_);

  if (map->find(ppn) == map->end())
  {
    debug(SWAPPING, "PageManager::removeIPT ppn %zu not found in the specified map\n", ppn);
    assert(0 && "PageManager::removeEntryIPT: ppn doesnt exist in IPT\n");
  }
  assert((*map)[ppn]->isLocked(pte) && "PageManager::removeEntryIPT: archmem is not locked\n");

  // Actually remove the pte from the entry, and delete the entry if theres no pte left
  (*map)[ppn]->removeEntry(pte);

  if ((*map)[ppn]->isEmpty())
  {
    delete (*map)[ppn];
    (*map).erase(ppn);
  }

  debug(SWAPPING, "PageManager::removeIPT: successfully removed from IPT\n");
}

void IPTManager::moveEntry(IPTMapType source, uint64 ppn_source, uint64 ppn_destination)
{
  debug(SWAPPING, "PageManager::moveEntry: moving entry ppn %zu to ppn %zu\n", ppn_source, ppn_destination);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "PageManager::moveEntry called but IPT not locked\n");
  assert(source == IPTMapType::NONE && "PageManager::moveEntry invalid parameter\n");

  ustl::map<uint64, IPTEntry*>* source_map = (source == IPTMapType::IPT_MAP ? &inverted_page_table_ : &swapped_page_map_);
  ustl::map<uint64, IPTEntry*>* destination_map = (source == IPTMapType::IPT_MAP ? &swapped_page_map_ : &inverted_page_table_);
  IPTEntry* entry = nullptr;

  if (source_map->find(ppn_source) == source_map->end())
  {
    debug(SWAPPING, "PageManager::moveEntry ppn %zu not found in the source map\n", ppn_source);
    assert(0 && "PageManager::moveEntry: The given ppn doesnt exist in the specifed map\n");
  }
  else
  {
    entry = (*source_map)[ppn_source];
    source_map->erase(ppn_source);
  }

  if (destination_map->find(ppn_destination) != destination_map->end())
  {
    debug(SWAPPING, "PageManager::moveEntry ppn %zu already exists in the destination map\n", ppn_destination);
    assert(0 && "PageManager::moveEntry: ppn_destination already exists in destination map\n");
  }
  else
  {
    assert(!entry && "PageManager::moveEntry: IPTEntry* is nullptr\n");
    (*destination_map)[ppn_destination] = entry;
    debug(SWAPPING, "PageManager::moveEntry: successfully moved entry\n");
  }
}


IPTMapType IPTManager::isInWhichMap(uint64 ppn)
{
  if (inverted_page_table_.find(ppn) != inverted_page_table_.end())
    return IPTMapType::IPT_MAP;
  if (swapped_page_map_.find(ppn) != swapped_page_map_.end())
    return IPTMapType::SWAPPED_PAGE_MAP;
  return IPTMapType::NONE;
}


template<typename... Args>
void ArchMemory::lockArchmemInOrder(Args... args)
{
    ustl::vector<Mutex*> vec = { args... };
    ustl::sort(vec.begin(), vec.end());

    // Lock the mutexes in order
    for(auto& m : vec) {
      assert(m && "Mutex for archmem is null");
      m->acquire();
    }
}