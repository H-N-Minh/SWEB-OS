#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "Mutex.h"

enum IPTMapType {RAM_MAP, DISK_MAP, NONE};
enum PRA_TYPE {RANDOM, NFU};

#include "ArchMemory.h"

typedef size_t ppn_t;
typedef size_t diskoffset_t;

class ArchMemory;


class IPTEntry {
public:
  size_t vpn_;
  ArchMemory* archmem_;

  IPTEntry(size_t vpn, ArchMemory* archmem);

  /**
   * check if the archmem of this entry is locked
  */
  bool isLocked();

  // TODO: getter func for getting PTE*

};


// TODO: IPT Manger must be made singleton and be initialized somewhere
class IPTManager {
public:
  // locking order: Ipt_lock -> disk_lock -> archmem_lock
  Mutex IPT_lock_;          // responsible for: ram_map_, disk_map_, pra_type_, swap_meta_data_
  ustl::multimap<ppn_t, IPTEntry*> ram_map_;
  ustl::multimap<diskoffset_t, IPTEntry*> disk_map_;
  PRA_TYPE pra_type_;       // NFU is default (in ctor). This attr belongs in IPTManager because it shares the IPT_lock_
  // key: ppn, value: counter for how often the page is used.
  // Key is managed by IPTManager, must be in sync with ram_map_.
  // Value is managed by SwappingThread
  ustl::map<ppn_t, uint32> swap_meta_data_;
  
  IPTManager();
  ~IPTManager();

  static IPTManager *instance();

  IPTEntry* lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);

  /**
   * Insert the PTE to the entry (at key == ppn) of the Inverted Page Table or the Swapped Page Map.
   * This does not accept inserting the same pte twice to the same ppn entry. (asserts fail)
   * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
   * @param ppn The ppn to insert (or the offset in disk for SWAPPED_PAGE_MAP)
   * @param vpn the virtual page that will be used to locate the right PTE
  */
  void insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem);

  /**
   * Remove a pte from an entry from the Inverted Page Table or the Swapped Page Map.
   * This does not accept removing a pte that does not exist at the specified entry. (asserts if this happens)
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
  ustl::vector<IPTEntry*> moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination);

  int getNumPagesInMap(IPTMapType maptype);

  bool isKeyInMap(size_t offset, IPTMapType maptype);

  /**
   * helper for insert, remove, moveEntry. This checks if an entry is already in the map
  */
  bool isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem);

  // ustl::vector<IPTEntry*> getPageInfosForPPN(size_t ppn); //TODOs

  /**
   * @return a vector of all unique keys in the ram_map_
  */
  ustl::vector<ppn_t> getUniqueKeysInRamMap();

  /**
   * @return the ppn of the page that will be swapped out. This is where the PRA is used
  */
  size_t findPageToSwapOut();

  /**
   * @return find all the values of a given key in the disk_map_ and put them into a vector
  */
  ustl::vector<IPTEntry*> getDiskEntriesFromKey(size_t disk_offset);

  /**
   * @return find all the values of a given key in the ram_map_ and put them into a vector
  */
  ustl::vector<IPTEntry*> getRamEntriesFromKey(size_t ppn);

  /**
   * Debug func, check if ppn of all archmem matches the key of ram_map_.
  */
  void checkRamMapConsistency();

  /**
   * Debug func, check if ppn of all archmem matches the key of disk_map_
  */
  void checkDiskMapConsistency();

  /**
   * Debug func, check if the swap_meta_data_ is in sync with ram_map_
  */
  void checkSwapMetaDataConsistency();


  private:
    static IPTManager* instance_;

    int pages_in_ram_ = 0;  //TODOs: at the moment also increases when shared
    int pages_on_disk_ = 0;  //TODOs: at the moment also increases when shared
  
};
