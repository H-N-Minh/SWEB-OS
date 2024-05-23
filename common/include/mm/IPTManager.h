#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "ArchMemory.h"
#include "PageManager.h"

typedef size_t ppn_t;
typedef size_t diskoffset_t;


enum IPTMapType {RAM_MAP, DISK_MAP, NONE};

class IPTEntry {
public:
  ppn_t ppn;
  size_t vpn;
  ArchMemory* archmem;

  IPTEntry(ppn_t ppn, size_t vpn, ArchMemory* archmem);

  /**
   * check if the archmem of this entry is locked
  */
  bool isLocked();

  // TODO: getter func for getting PTE*

};


// TODO: IPT Manger must be made singleton and be initialized somewhere
class IPTManager {
public:
  Mutex IPT_lock_;          // lock both ram_map_ and disk_map_
  ustl::multimap<ppn_t, ustl::shared_ptr<IPTEntry>> ram_map_;
  ustl::multimap<diskoffset_t, ustl::shared_ptr<IPTEntry>> disk_map_;
  
  IPTManager();

  ustl::shared_ptr<IPTEntry> lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  ustl::shared_ptr<IPTEntry> lookupEntryInDisk(diskoffset_t diskOffset);

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
  void moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination);

  /**
   * based on the given address, check which map the entry is in currently.
   * @return IPTMapType that indicates which map currently has this ppn as key. 0 if not found in both.
  */
  IPTMapType isInWhichMap(size_t ppn);

  /**
   * Locks the archmems in the order of lowest to highest address of the Mutex
  */
  template<typename... Args>
  void lockArchmemInOrder(Args... args);
};
