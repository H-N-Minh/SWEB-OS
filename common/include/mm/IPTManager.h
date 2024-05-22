#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "ArchMemory.h"
#include "PageManager.h"

typedef int ppn_t;
typedef int diskoffset_t;


enum IPTMapType {IPT_MAP, SWAPPED_PAGE_MAP, NONE};

class IPTEntry {
public:
  ppn_t ppn;
  size_t vpn;
  ArchMemory* archmem;

  IPTEntry(ppn_t ppn, size_t vpn, ArchMemory* archmem) : ppn(ppn), vpn(vpn), archmem(archmem) {}

  /**
   * check if the archmem of the specified PTE is locked by currentThread or not
  */
  bool isLocked(PageTableEntry* pte);

};

class IPTManager {
public:
  ustl::multimap<ppn_t, ustl::shared_ptr<IPTEntry>> ramMap;
  ustl::map<diskoffset_t, ustl::shared_ptr<IPTEntry>> diskMap;


  ustl::shared_ptr<IPTEntry> createIPTEntry(int ppn, size_t vpn, ArchMemory *archmem);
  void addEntryToRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  void addEntryToDisk(diskoffset_t diskOffset, size_t vpn, ArchMemory* archmem);
  ustl::shared_ptr<IPTEntry> lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  ustl::shared_ptr<IPTEntry> lookupEntryInDisk(diskoffset_t diskOffset);
  void removeEntryFromRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  void swapOutPage(ppn_t ppn, diskoffset_t diskOffset);
  void swapInPage(diskoffset_t diskOffset, ppn_t ppn);




  // Inverted Page Table
  Mutex IPT_lock_;          // lock both inverted_page_table_ and swapped_page_map_
  ustl::map<ppn_t , IPTEntry*> inverted_page_table_;  // map<ppn, map<PTE*, archmem_lock> >
  ustl::map<diskoffset_t , IPTEntry*> swapped_page_map_;  // map<offset in disk, map<PTE*, archmem_lock> >

  /**
   * Insert the PTE to the entry (at key == ppn) of the Inverted Page Table or the Swapped Page Map.
   * This does not accept inserting the same pte twice to the same ppn entry. (asserts fail)
   * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
   * @param ppn The ppn to insert (or the offset in disk for SWAPPED_PAGE_MAP)
   * @param pte The PageTableEntry to insert
   * @param archmem_lock The lock of the archmem that the PTE belongs to
  */
  void insertEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte, Mutex* archmem_lock);

  /**
   * Remove a pte from an entry from the Inverted Page Table or the Swapped Page Map.
   * This does not accept removing a pte that does not exist at the specified entry. (asserts if this happens)
   * @param map_type IPT_MAP for insertion to inverted_page_table_, SWAPPED_PAGE_MAP for swapped_page_map_
   * @param ppn The ppn to remove (or the offset in disk for SWAPPED_PAGE_MAP)
   * @param pte The PageTableEntry to remove
  */
  void removeEntryIPT(IPTMapType map_type, uint64 ppn, PageTableEntry* pte);

  /**
   * Remove an entry from the source map and insert it to the destination map.
   * @param source the map that the entry is in currently. Destination is then automatically the other map
   * @param ppn_source The location of the entry currently
   * @param ppn_destination Location of the new entry in the destination map. This new entry must be empty.
  */
  void moveEntry(IPTMapType source, uint64 ppn_source, uint64 ppn_destination);

  /**
   * based on the given address, check which map the entry is in currently.
   * @return IPTMapType that indicates which map currently has this ppn as key. 0 if not found in both.
  */
  IPTMapType isInWhichMap(uint64 ppn);

};
