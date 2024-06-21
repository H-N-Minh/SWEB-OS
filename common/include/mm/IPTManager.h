#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "Mutex.h"
#include "IPTEntry.h"

enum IPTMapType {RAM_MAP, DISK_MAP, NONE};
enum PRA_TYPE {RANDOM, NFU, SECOND_CHANGE};

// class IPTEntry;
class ArchMemory;

typedef size_t fake_ppn_t;    // index of the
typedef size_t vpn_t;
typedef size_t ppn_t;
typedef size_t diskoffset_t;

class IPTManager
{
private:
  static IPTManager* instance_;

public:
  // locking order: Ipt_lock -> disk_lock -> archmem_lock
  Mutex IPT_lock_;          // responsible for: ram_map_, disk_map_, pra_type_
  ustl::map<ppn_t, IPTEntry*> ram_map_;
  ustl::map<diskoffset_t, IPTEntry*> disk_map_;
  PRA_TYPE pra_type_;       // NFU is default (in ctor). This attr belongs in IPTManager because it shares the IPT_lock_

  ustl::vector<uint32> fifo_ppns;
  unsigned last_index_ = 0;

  // When a page is set as shared, it is not assigned a ppn yet until a PF happens. Without ppn, it cant be added to IPT table.
  // Therefore this map exists. It assigns a fake ppn temprorarily, until the page is actually allocated a real ppn.
  ustl::map<ArchMemory*, ustl::map<vpn_t, fake_ppn_t>> fake_ppn_map_;

  // the map to tracks backwards from fake ppn to all the archmem that is sharing this fake ppn. 
  // This is used when the shared page is assigned a real ppn, then all the archmem that is sharing the page should be updated with the real ppn
  ustl::multimap<fake_ppn_t, ArchmemIPT*> inverted_fake_ppn_;

  fake_ppn_t fake_ppn_counter_ = 0; // for generating fake ppn

  Mutex fake_ppn_lock_; // responsible for fake_ppn_map_ and inverted_fake_ppn_ and fake_ppn_counter_

  IPTManager();
  ~IPTManager();

  static IPTManager *instance();


  /**
   * Insert the PTE to the entry (at key == ppn) of the ram map or disk map.
   * This does not accept inserting the same archmem twice to the same ppn entry. (asserts fail)
   * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
   * @param ppn The ppn to insert (or the offset in disk for SWAPPED_PAGE_MAP)
   * @param vpn the virtual page that will be used to locate the right PTE
  */
  void insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem);


  /**
   * Remove an entry from the ram map or disk map
   * This does not accept removing an entry that does not exist in the map. (asserts if this happens)
   * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
   * @param ppn The ppn to remove (or the offset in disk for SWAPPED_PAGE_MAP)
   * @param vpn the virtual page that will be used to locate the right PTE
  */
  void removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem);


  /**
   * Remove an entry from the source map and insert it to the destination map.
   * @param source the map that the entry is in currently. Destination is then automatically the other map
   * @param ppn_source The location of the entry currently
   * @param ppn_destination Location of the new entry in the destination map. This new entry must be empty.
  */
  void moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination);


  void removeEntry(IPTMapType map_type, size_t ppn);

  /**
   * Getter for debugging info, doesnt use any lock but who cares
  */
  int getNumPagesInMap(IPTMapType maptype);

  /**
   * Check if a page is in a map
  */
  bool isKeyInMap(size_t offset, IPTMapType maptype);

  /**
   * helper for insert, remove, moveEntry. This checks if an entry with exact Archmem and ppn is already in the map
  */
  bool isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem, size_t vpn);

  /**
   * @return the ppn of the page that will be swapped out. This is where the PRA is used
  */
  size_t findPageToSwapOut();


  /**
   * Debug func, check if ppn of all archmem matches the key of ram_map_.
  */
  void checkRamMapConsistency();

  /**
   * Debug func, check if ppn of all archmem matches the key of disk_map_
  */
  void checkDiskMapConsistency();

  /**
   * Debug func, check if random generator is actually random
  */
  void debugRandomGenerator();

  /**
   * Is called when a new shared page is created. Adding entry to both fake_ppn_map_ and inverted_fake_ppn_
  */
  void insertFakePpnEntry(ArchMemory* archmem, size_t vpn);

  /**
   * Is called when a page fault happens, and the shared page receives a real ppn, then all the archmem that is sharing the page should be updated with the real ppn
   * this also removes entry from fake_ppn_map_ and inverted_fake_ppn_
  */
  void mapRealPPN(size_t ppn, size_t vpn, ArchMemory* arch_memory, ustl::vector<uint32>& preallocated_pages);

  /**
   * Is called when archmemory copy constructor. Add the child to the same shared page as the parent. Adding to both fake_ppn_map_ and inverted_fake_ppn_
  */
  void copyFakedPages(ArchMemory* parent, ArchMemory* child);

  /**
   * this is for when a shared page is unmapped when its not even mapped yet. removing entry from fake_ppn_map_
  */
  void unmapOneFakePPN(size_t vpn, ArchMemory* arch_memory);

};
