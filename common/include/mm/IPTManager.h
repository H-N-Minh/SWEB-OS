#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "Mutex.h"

enum IPTMapType {RAM_MAP, DISK_MAP, NONE};
enum PRA_TYPE {RANDOM, AGING};

#include "ArchMemory.h"

typedef size_t ppn_t;
typedef size_t diskoffset_t;

class ArchMemory;


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
  Mutex IPT_lock_;          // lock both ram_map_ and disk_map_ and also pra_type_
  ustl::multimap<ppn_t, IPTEntry*> ram_map_;
  ustl::multimap<diskoffset_t, IPTEntry*> disk_map_;
  PRA_TYPE pra_type_;       // aging is default (in ctor)
  
  IPTManager();
  ~IPTManager();

  static IPTManager *instance();

  IPTEntry* lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  IPTEntry* lookupEntryInDisk(diskoffset_t diskOffset);

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

  bool KeyisInMap(size_t offset, IPTMapType maptype);

  // ustl::vector<IPTEntry*> getPageInfosForPPN(size_t ppn); //TODOs

  /**
   * @return a vector of all unique keys in the ram_map_
  */
  ustl::vector<ppn_t> getUniqueKeysInIPT();

  /**
   * @return the ppn of the page that will be swapped out. This is where the PRA is used
  */
  size_t findPageToSwapOut();


  private:
    static IPTManager* instance_;

    int pages_in_ram_ = 0;  //TODOs: at the moment also increases when shared
    int pages_on_disk_ = 0;  //TODOs: at the moment also increases when shared
  
};
