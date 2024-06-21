#include "IPTManager.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "SwappingThread.h"
#include "Syscall.h"
#include "Thread.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"
#include "Syscall.h"
#include "ulimits.h"
#include "uset.h"
#include "SwappingManager.h"
#include "ArchMemory.h"
#include "SwappingThread.h"
#include "SharedMemManager.h"


#define INVALID_PPN 0
#define UINT32_MAX 0xFFFFFFFF

class ArchmemIPT;

////////////////////// IPTManager //////////////////////
IPTManager* IPTManager::instance_ = nullptr;

IPTManager::IPTManager()
  : IPT_lock_("IPTManager::IPT_lock_"), fake_ppn_lock_("IPTManager::fake_ppn_lock_")
{
  assert(!instance_);
  instance_ = this;
  pra_type_ = PRA_TYPE::NFU;
}

IPTManager::~IPTManager()
{
  //delete values in ipt_disk
  for (auto& map_entry : ram_map_)
  {
    delete map_entry.second;
  }
  ram_map_.clear();

  //delete values in ipt_ram
  for (auto& map_entry : disk_map_)
  {
    delete map_entry.second;
  }
  disk_map_.clear();
}

IPTManager* IPTManager::instance()
{
  return instance_;
}

size_t randomNumGenerator()
{
  size_t time_stamp = (size_t) Syscall::get_current_timestamp_64_bit();
  return time_stamp / 73;
}

// only for debugging, used to see if the random number generator is working
void IPTManager::debugRandomGenerator()
{
  ustl::map<int, int> myMap;

  for (size_t i = 0; i < 2000000; i++)
  {
    size_t randomNum = randomNumGenerator();
    int randomNum_resized = randomNum % 1024;
    if (myMap.find(randomNum_resized) != myMap.end())
    {
      myMap[randomNum_resized]++;
    }
    else
    {
      myMap[randomNum_resized] = 1;
    }
  }

  ustl::vector<ustl::pair<int, int>> numberPairs;
  for (const auto& pair : myMap) {
    numberPairs.push_back(pair);
  }
  struct Compare {
    bool operator()(const ustl::pair<int, int>& a, const ustl::pair<int, int>& b) {
      return a.second < b.second;
    }
  };
  ustl::sort(numberPairs.begin(), numberPairs.end(), Compare());

  debug(MINH, "\n\nsort by number of time showing up\n");
  for (const auto& pair : numberPairs)
  {
    debug(MINH, "number: %d, count: %d\n", pair.first, pair.second);
  }

  struct Compare2 {
    bool operator()(const ustl::pair<int, int>& a, const ustl::pair<int, int>& b) {
      return a.first < b.first;
    }
  };
  ustl::sort(numberPairs.begin(), numberPairs.end(), Compare2());

  debug(MINH, "\n\nsort by value of generated number\n");
  for (const auto& pair : numberPairs)
  {
    debug(MINH, "number: %d, count: %d\n", pair.first, pair.second);
  }
}

size_t IPTManager::findPageToSwapOut()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::findPageToSwapOut called but IPT not locked\n");

  size_t ppn_retval = INVALID_PPN;

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();

  if (pra_type_ == PRA_TYPE::RANDOM)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA RANDOM\n");

    size_t random_num = randomNumGenerator();
    // debug(MINH, "IPTManager::findPageToSwapOut: random num : %zu\n", random_num);

    if(ram_map_.size() == 0)
    {
      assert(0 && "No page in ram");
    }
    size_t random_ipt_index = random_num % ram_map_.size();

    size_t counter = 0;
    for (const auto& pair : ram_map_)
    {
      if(counter == random_ipt_index)
      {
        ppn_retval = pair.first;
        break;
      }
      counter++;
    }
    debug(IPT, "IPTManager::findPageToSwapOut: Found random page to swap out: ppn=%ld\n", ppn_retval);
  }
  else if (pra_type_ == PRA_TYPE::NFU)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA NFU\n");
    uint32 min_counter = UINT32_MAX;
    ustl::vector<uint32> min_ppns;    // vector of all pages with the minimum counter

    // go through ram_map_ and find the page with the lowest access counter
    assert(ram_map_.size() > 0 && "IPTManager::findPageToSwapOut: ram_map_ is empty. this should never happen\n");
    for(auto& pair : ram_map_)
    {
      ppn_t key = pair.first;
      uint32 counter = pair.second->access_counter_;
      if (counter < min_counter && counter != 0)
      {
        min_counter = counter;
        min_ppns.clear();
        ppn_retval = key;
        min_ppns.push_back(key);
      }
      else if (counter == min_counter)
      {
        min_ppns.push_back(key);
      }
    }
    debug(IPT, "IPTManager::findPageToSwapOut: Found %zu pages with the minimum counter: %d\n", min_ppns.size(), min_counter);
    if (min_ppns.size() > 1)
    {
      debug(IPT, "IPTManager::findPageToSwapOut: Multiple pages with the minimum counter. Randomly selecting one\n");
      size_t random_num = randomNumGenerator();
      size_t random_index = random_num % min_ppns.size();
      ppn_retval = min_ppns[random_index];
    }

    debug(SWAPPING, "IPTManager::findPageToSwapOut: Found page to swap out: ppn=%ld, counter=%d\n", ppn_retval, min_counter);
  }
  else if(pra_type_ == PRA_TYPE::SECOND_CHANGE)
  {
    if(fifo_ppns.empty())
    {
      assert(0);
    }
    int counter = 0;

    while(ppn_retval == INVALID_PPN && counter < 3)
    {
      counter++;

      for(size_t i = 0; i <  fifo_ppns.size() && ppn_retval == INVALID_PPN; i++)
      {
        auto& ppn = fifo_ppns.at(i);
        assert(isKeyInMap(ppn, IPTMapType::RAM_MAP) && "selected page need to be in ram");
        IPTEntry* entry = ram_map_[ppn];
        for(auto& archmem_ipt : entry->getArchmemIPTs())
        {
          ArchMemory* archmem = archmem_ipt->archmem_;
          archmem->archmemory_lock_.acquire();
          size_t vpn = archmem_ipt->vpn_;
          if(!archmem->isBitSet(vpn, BitType::SECONDCHANGE, true))
          {
            archmem->setSecondChange(vpn);
            archmem->archmemory_lock_.release();
          }
          else
          {
            ppn_retval = ppn;
            archmem->archmemory_lock_.release();
            break;
          }
        }
      }
    }

    if(ppn_retval != INVALID_PPN)
    {
      auto it = ustl::find(fifo_ppns.begin(), fifo_ppns.end(), ppn_retval);
      if (it != fifo_ppns.end())
      {
        fifo_ppns.erase(it);

        for(auto& archmem_ipt : ram_map_[ppn_retval]->getArchmemIPTs())
        {
          ArchMemory* archmem = archmem_ipt->archmem_;
          size_t vpn = archmem_ipt->vpn_;
          archmem->archmemory_lock_.acquire();
          archmem->resetSecondChange(vpn);
          archmem->archmemory_lock_.release();
        }
      }
      else
      {
        assert(0 && "PPN not found in fifo_ppns vector");
      }
    }
    else
    {
      assert(0);
    }
  }
  assert(isKeyInMap(ppn_retval, IPTMapType::RAM_MAP) && "selected page need to be in ram");
  assert(ppn_retval != INVALID_PPN && "IPTManager::findPageToSwapOut: failed to find a valid ppn\n");
  return ppn_retval;
}

void IPTManager::insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(SWAPPING, "IPTManager::insertEntryIPT: inserting ppn: %zu, vpn: %zu, archmem: %p to %s\n", ppn, vpn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertEntryIPT called without fully locking\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  // Error checking: entry should not already exist
  if (isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::insertEntryIPT: Entry (ppn: %zu, archmem: %p) already exists in %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::insertEntryIPT: Entry already exists in map\n");
  }

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();

  debug(IPT, "IPTManager::insertEntryIPT: Entry does not exist in %s yet, seems valid. Inserting\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  if(isKeyInMap(ppn, map_type))
  {
    IPTEntry* entry = (*map)[ppn];
    assert(entry && "IPTManager::insertIPT: entry is null");

    entry->addArchmemIPT(vpn, archmem);

    if(map_type == IPTMapType::RAM_MAP)
    {
      auto it = ustl::find(fifo_ppns.begin(), fifo_ppns.end(), ppn);
      if (it != fifo_ppns.end())
      {
        fifo_ppns.erase(it);
      }
      fifo_ppns.push_back(ppn);
    }
  }
  else
  {
    (*map)[ppn] = new IPTEntry();
    IPTEntry* entry = (*map)[ppn];
    entry->addArchmemIPT(vpn, archmem);
    if(map_type == IPTMapType::RAM_MAP)
    {
      fifo_ppns.push_back(ppn);
    }
  }

  if (!isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::insertEntryIPT: Inserting Entry (ppn: %zu, archmem: %p) failed in %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::insertEntryIPT: Insert entry failed\n");
  }

  debug(IPT, "IPTManager::insertIPT: successfully inserted to IPT\n");
}

void IPTManager::removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(IPT, "IPTManager::removeEntryIPT: removing ppn: %zx, archmem: %p from map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntryIPT called but not fully locked\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  // Error checking that the entry does exist in the map before removing
  if (!isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::removeEntryIPT Entry (ppn %zu, archmem %p) not found in the map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::removeEntryIPT: ppn doesnt exist in map\n");
  }

  // remove the item and update debug info
  debug(IPT, "IPTManager::removeEntryIPT: Entry found in map %s, seems valid. Removing\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  IPTEntry* entry = (*map)[ppn];
  entry->removeArchmemIPT(vpn, archmem);
  if (entry->isEmpty())
  {
    delete entry;
    map->erase(ppn);

    if(map_type == IPTMapType::RAM_MAP)
    {
      auto it = ustl::find(fifo_ppns.begin(), fifo_ppns.end(), ppn);
      if (it != fifo_ppns.end())
      {
        fifo_ppns.erase(it);
      }
      else
      {
        assert(0 && "PPN not found in fifo_ppns vector");
      }
    }
  }

  if (isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::removeEntryIPT: Entry (ppn: %zu, archmem: %p) not removed from %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::removeEntryIPT: Removing failed\n");
  }

  debug(IPT, "IPTManager::removeIPT: successfully removed from IPT\n");

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
}

void IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");
  // TODO: assert that all archmem of the entry are locked

  auto* source_map                  = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map             = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);
  const char* source_as_string      = (source == IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");
  const char* destination_as_string = (source == IPTMapType::RAM_MAP ? "DISK-MAP" : "RAM-MAP");
  IPTMapType destination   = (source == IPTMapType::RAM_MAP ? IPTMapType::DISK_MAP : IPTMapType::RAM_MAP);

  debug(SWAPPING, "IPTManager::moveEntry: moving entry at offset %zu in %s to offset %zu in %s\n", ppn_source, source_as_string, ppn_destination, destination_as_string);
  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();


  // Check if the move is valid (entry to be moved exists in source map, and does not exist in destination map)
  // also check if the archmem are locked
  if (!isKeyInMap(ppn_source, source))
  {
    debug(IPT, "IPTManager::moveEntry: Entry to be moved (ppn %zu) not found in source map %s\n", ppn_source, source_as_string);
    assert(0 && "IPTManager::moveEntry: Entry to be moved not found in source map\n");
  }

  IPTEntry* entry = (*source_map)[ppn_source];
  assert(entry && "IPTManager::moveEntry: entry is null");
  ustl::vector<ArchmemIPT*> archmemIPTs_vector = entry->getArchmemIPTs();
  assert(archmemIPTs_vector.size() > 0 && "IPTManager::moveEntry: archmemIPTs_vector is empty even tho the IPTentry exists\n");

  for (auto archmemIPT : archmemIPTs_vector)
  {
    assert(archmemIPT->isLockedByUs() && "IPTManager::moveEntry: ArchMemory not locked while moving entry\n");
    assert(!isEntryInMap(ppn_destination, destination, archmemIPT->archmem_,  archmemIPT->vpn_) && "IPTManager::moveEntry: Entry to be moved already exists in destination map\n");
  }
  debug(SWAPPING, "IPTManager::moveEntry: Entry to be moved seems valid, moving now\n");

  // Moving entries
  (*destination_map)[ppn_destination] = entry;
  source_map->erase(ppn_source);
  entry->access_counter_ = 0;

  if(source != IPTMapType::RAM_MAP)
  {
    fifo_ppns.push_back(ppn_destination);
  }

  if (!isKeyInMap(ppn_destination, destination) && isKeyInMap(ppn_source, source))
  {
    assert(0 && "IPTManager::moveEntry: Move failed\n");
  }
}

void IPTManager::copyEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
	assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::copyEntry called but IPT not locked\n");

	auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
	auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);

	IPTEntry* entry = (*source_map)[ppn_source];
	assert(entry);

	// copy the entry to the destination and the source remains untouched
	(*destination_map)[ppn_destination] = new IPTEntry(*entry);
}

void IPTManager::finalizePreSwappedEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
	assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::finalizePreSwappedEntry called but IPT not locked\n");

	// get reference to source map and destination map
	auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
	auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);

	IPTEntry* entry = (*source_map)[ppn_source];

	// add entry to the destination map
	(*destination_map)[ppn_destination] = entry;
	// remove entry from the source map
	source_map->erase(ppn_source);

	// Check if finalize operation failed or not
	if (isKeyInMap(ppn_source, source) || !isKeyInMap(ppn_destination, source == IPTMapType::RAM_MAP ? IPTMapType::DISK_MAP : IPTMapType::RAM_MAP))
	{
		assert(0 && "IPTManager::finalizePreSwappedEntry: finalize failed\n");
	}
}

void IPTManager::removeEntry(IPTMapType map_type, size_t ppn)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntry called but IPT not locked\n");

  if (!isKeyInMap(ppn, map_type))
  {
    assert(0 && "IPTManager::removeEntry: PPN not found in map\n");
  }
  auto& map = (map_type == IPTMapType::RAM_MAP ? ram_map_ : disk_map_);

  map.erase(ppn);

  if (isKeyInMap(ppn, map_type))
  {
    assert(0 && "IPTManager::removeEntry: PPN after removing still in map\n");
  }

}


bool IPTManager::isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem, size_t vpn)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isEntryInMap called but IPT not locked\n");
  auto* map = (maptype == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  auto it = map->find(ppn);
  if (it == map->end())
  {
    return false;
  }
  else
  {
    return it->second->isArchmemExist(archmem, vpn);
  }
}

bool IPTManager::isKeyInMap(size_t offset, IPTMapType maptype)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isKeyInMap called but IPT not locked\n");
  auto* map = (maptype == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  if (map->find(offset) == map->end())
  {
    return false;
  }
  else
  {
    return true;
  }
}

int IPTManager::getNumPagesInMap(IPTMapType maptype)
{
  if(maptype == IPTMapType::RAM_MAP)
  {
    return ram_map_.size();
  }
  else
  {
    return disk_map_.size();
  }
}

void IPTManager::checkRamMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkRamMapConsistency called but IPT not locked\n");
  assert(ram_map_.size() && "IPTManager::checkRamMapConsistency: ram_map_ is empty, unlikely to happen\n");

  for (auto it = ram_map_.begin(); it != ram_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    assert(ipt_entry && "checkRampMapConsistency: No IPTEntry");
    ustl::vector<ArchmemIPT*> archmemIPTs_vector = ipt_entry->getArchmemIPTs();
    assert(archmemIPTs_vector.size() && "checkRampMapConsistency: No ArchmemIPT (empty archmem vector), but IPTEntry still exists in ram_map_");

    for (auto archmemIPT : archmemIPTs_vector)
    {
      ArchMemory* entry_arch = archmemIPT->archmem_;
      size_t vpn = archmemIPT->vpn_;
      assert(entry_arch && "No archmem in ArchmemIPT");

      // this locking will not solve deadlock completely, but this is debug func so who cares
      int locked_by_us = 0;
      if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
      {
        entry_arch->archmemory_lock_.acquire();
        locked_by_us = 1;
      }

      ArchMemoryMapping mapping = entry_arch->resolveMapping(vpn);
      PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
      assert(pt_entry && "checkRampMapConsistency: No pagetable entry");

      if(!pt_entry->present)
      {
        assert(0 && "checkRampMapConsistency: page is not present, but exists in ram_map_\n");
      }
      else
      {
        assert(pt_entry->page_ppn == key && "checkRampMapConsistency: ppn in ram_map_ (key) does not match ppn in ArchMemory\n");
      }

      if (locked_by_us)
      {
        entry_arch->archmemory_lock_.release();
      }

    }
  }
}

void IPTManager::checkDiskMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkDiskMapConsistency called but IPT not locked\n");

  for (auto it = disk_map_.begin(); it != disk_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    assert(ipt_entry && "checkRampMapConsistency: No IPTEntry");
    ustl::vector<ArchmemIPT*> archmemIPTs_vector = ipt_entry->getArchmemIPTs();
    assert(archmemIPTs_vector.size() && "checkRampMapConsistency: No ArchmemIPT (empty archmem vector), but IPTEntry still exist in diskmap");

    for (auto archmemIPT : archmemIPTs_vector)
    {
      ArchMemory* entry_arch = archmemIPT->archmem_;
      size_t vpn = archmemIPT->vpn_;
      assert(entry_arch && "No archmem in ArchmemIPT");

      // this locking will not solve deadlock completely, but this is debug func so who cares
      int locked_by_us = 0;
      if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
      {
        entry_arch->archmemory_lock_.acquire();
        locked_by_us = 1;
      }

      ppn_t disk_offset = (ppn_t) entry_arch->getDiskLocation(vpn);
      assert(disk_offset != 0);
      assert(disk_offset && "checkRampMapConsistency: disk_offset is 0\n");
      assert(key == disk_offset && "checkRampMapConsistency: ppn in disk_map_ (key) does not match disk offset in ArchMemory\n");
      if (locked_by_us)
      {
        entry_arch->archmemory_lock_.release();
      }
    }
  }
}


void IPTManager::insertFakePpnEntry(ArchMemory* archmem, size_t vpn)
{
  assert(fake_ppn_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertFakePpnEntry called but fake_ppn_lock_ not locked\n");

  fake_ppn_t new_ppn = fake_ppn_counter_;

  // Adding entry to the fake_ppn_map_
  debug(IPT, "IPTManager::insertFakePpnEntry: inserting archmem: %p, vpn: %zu to fake_ppn_map_\n", archmem, vpn);
  auto it = fake_ppn_map_.find(archmem);
  if (it != fake_ppn_map_.end())
  {
    debug(IPT, "IPTManager::insertFakePpnEntry: archmem exists in fake_ppn_map_\n");
    auto& sub_map = it->second;
    if (sub_map.find(vpn) != sub_map.end())
    {
      assert(0 && "IPTManager::insertFakePpnEntry: vpn already exists in the sub map");
    }
    else
    {
      sub_map[vpn] = new_ppn;
    }
  }
  else
  {
    debug(IPT, "IPTManager::insertFakePpnEntry: archmem does not exist yet in fake_ppn_map_, creating new\n");
    fake_ppn_map_[archmem][vpn] = new_ppn;
  }

  // adding entry to the inverted fake ppn map
  auto it2 = inverted_fake_ppn_.find(new_ppn);
  if (it2 != inverted_fake_ppn_.end())
  {
    debug(IPT, "IPTManager::insertFakePpnEntry: ppn %zu already exists in inverted_fake_ppn_\n", new_ppn);
    assert(0 && "IPTManager::insertFakePpnEntry: ppn already exists in inverted_fake_ppn_\n");
  }
  else
  {
    ArchmemIPT* new_arch_ipt = new ArchmemIPT(vpn, archmem);
    inverted_fake_ppn_.insert({new_ppn, new_arch_ipt});
  }

  fake_ppn_counter_++;
}


void IPTManager::copyFakedPages(ArchMemory* parent, ArchMemory* child)
{
  assert(fake_ppn_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::copyFakedPages called but fake_ppn_lock_ not locked\n");

  // check if the parent archmem exists in the map (it should)
  auto it = fake_ppn_map_.find(parent);
  if (it != fake_ppn_map_.end())
  {
    auto& sub_map = it->second;
    for (auto& pair : sub_map)
    {
      size_t vpn = pair.first;
      fake_ppn_t mutual_fake_ppn = pair.second;

      // adding child archmem to the fake_ppn_map_
      debug(IPT, "IPTManager::copyFakedPages: adding child archmem: %p, vpn: %zu to fake_ppn_map_\n", child, vpn);
      fake_ppn_map_[child][vpn] = mutual_fake_ppn;

      // adding child archmem to the inverted_fake_ppn_
      ArchmemIPT* new_arch_ipt = new ArchmemIPT(vpn, child);
      inverted_fake_ppn_.insert({mutual_fake_ppn, new_arch_ipt});
    }
  }
}

// TODO: lock all the archmemory
void IPTManager::mapRealPPN(size_t ppn, size_t vpn, ArchMemory* arch_memory, ustl::vector<uint32>& preallocated_pages)
{
  assert(fake_ppn_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::mapRealPPN called but fake_ppn_lock_ not locked\n");
  fake_ppn_t mutual_fake_ppn = 0;

  // find if the archmemory exists in the fake_ppn_map_, it should
  auto archMemoryIt = fake_ppn_map_.find(arch_memory);
  if (archMemoryIt != fake_ppn_map_.end())
  {
    auto& sub_map = archMemoryIt->second;
    if (sub_map.find(vpn) != sub_map.end())
    {
      mutual_fake_ppn = sub_map[vpn];
    }
    else
    {
      assert(0 && "IPTManager::mapRealPPN:  vpn not found in fake_ppn_map_\n");
    }
  }
  else
  {
    assert(0 && "IPTManager::mapRealPPN: archmem not found in fake_ppn_map_\n");
  }

  // go through every archmem in inverted_fake_ppn_ and update with the real ppn
  for (auto it = inverted_fake_ppn_.begin(); it != inverted_fake_ppn_.end(); ++it)
  {
    if (it->first == mutual_fake_ppn)
    {
      ArchmemIPT* archmem_ipt = it->second;
      assert(archmem_ipt && "IPTManager::mapRealPPN: archmem_ipt is null");
      ArchMemory* temp_archmem = archmem_ipt->archmem_;
      assert(temp_archmem && "IPTManager::mapRealPPN: archmem is null");
      size_t temp_vpn = archmem_ipt->vpn_;

      if (temp_archmem != arch_memory)
      {
        temp_archmem->archmemory_lock_.acquire();
      }


      bool rv = temp_archmem->mapPage(temp_vpn, ppn, 1, preallocated_pages);
      assert(rv == true);

      // setting up the bits
      SharedMemManager* smm = temp_archmem->shared_mem_manager_;
      assert(smm && "IPTManager::mapRealPPN: shared_mem_manager_ is null");
      SharedMemEntry* entry = smm->getSharedMemEntry(temp_vpn * PAGE_SIZE);
      smm->setProtectionBits(entry, temp_archmem, temp_vpn);
      temp_archmem->setSharedBit(temp_vpn);

      if (temp_archmem != arch_memory)
      {
        temp_archmem->archmemory_lock_.release();
      }

      // delete the entry in inverted_fake_ppn_
      delete it->second;
      inverted_fake_ppn_.erase(it);
      it--;

      // delete the entry in fake_ppn_map_
      fake_ppn_map_[temp_archmem].erase(temp_vpn);
      if (fake_ppn_map_[temp_archmem].size() == 0)
      {
        fake_ppn_map_.erase(temp_archmem);
      }

    }
  }
}


void IPTManager::unmapOneFakePPN(size_t vpn, ArchMemory* arch_memory)
{
  assert(fake_ppn_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::unmapOneFakePPN called but fake_ppn_lock_ not locked\n");

  // find if the archmemory exists in the fake_ppn_map_, it should
  auto it = fake_ppn_map_.find(arch_memory);
  if (it != fake_ppn_map_.end())
  {
    auto& sub_map = it->second;
    if (sub_map.find(vpn) != sub_map.end())
    {
      fake_ppn_t mutual_fake_ppn = sub_map[vpn];
      // removing from fake_ppn_map
      sub_map.erase(vpn);
      if (sub_map.size() == 0)
      {
        fake_ppn_map_.erase(it);
      }

      // removing from inverted_fake_ppn_
      for (auto it2 = inverted_fake_ppn_.begin(); it2 != inverted_fake_ppn_.end(); ++it2)
      {
        if (it2->first == mutual_fake_ppn)
        {
          ArchmemIPT* archmem_ipt = it2->second;
          assert(archmem_ipt && "IPTManager::unmapOneFakePPN: archmem_ipt is null");
          ArchMemory* temp_archmem = archmem_ipt->archmem_;
          assert(temp_archmem && "IPTManager::unmapOneFakePPN: archmem is null");
          size_t temp_vpn = archmem_ipt->vpn_;

          if (temp_archmem == arch_memory && temp_vpn == vpn)
          {
            // delete the entry in inverted_fake_ppn_
            delete it2->second;
            inverted_fake_ppn_.erase(it2);
            break;
          }
        }
      }
    }
    else
    {
      assert(0 && "IPTManager::unmapOneFakePPN: vpn not found in fake_ppn_map_\n");
    }
  }
  else
  {
    assert(0 && "IPTManager::unmapOneFakePPN: archmem not found in fake_ppn_map_\n");
  }
}