#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "Mutex.h"
#include "IPTEntry.h"

#define PRESWAPTHRESHOLD 80 //80%
#define MAX_PRESWAP_PAGES 20
enum IPTMapType {RAM_MAP, DISK_MAP, NONE};
enum PRA_TYPE {RANDOM, NFU};

// class IPTEntry;
class ArchMemory;

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
  bool isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem);

  /**
   * @return the ppn of the page that will be swapped out. This is where the PRA is used
  */
  size_t findPageToSwapOut();
  size_t findPageToSwapOutRandom();
  size_t findPageToSwapOutNFU();


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
  static void debugRandomGenerator();


  // tobe removed
  bool isThereAnyPageToSwapOut();

  //preswapping
  static bool ENABLE_PRE_SWAP; //true = used, false = not used
};
