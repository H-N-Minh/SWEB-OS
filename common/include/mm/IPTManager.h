#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "Mutex.h"
#include "IPTEntry.h"


enum IPTMapType {RAM_MAP, DISK_MAP, NONE};
enum PRA_TYPE {RANDOM, NFU, SIMPLE};

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
  Mutex IPT_lock_;          // responsible for: ram_map_, disk_map_, pra_type_, unmapped_shared_pages_
  ustl::map<ppn_t, IPTEntry*> ram_map_;
  ustl::map<diskoffset_t, IPTEntry*> disk_map_;
  PRA_TYPE pra_type_;       // NFU is default (in ctor). This attr belongs in IPTManager because it shares the IPT_lock_

  // When a page is set as shared, it is not assigned a ppn yet until a PF happens. Without ppn, it cant be added to IPT table.
  // Therefore this vector exists. Each sub-vector is responsible for 1 shared page, contains all archmem that all mapped to this shared page. 
  // Therefore, the vector unmap_shared_pages_ is a vector of all shared pages accross all processes, that are not yet allocated a ppn
  ustl::vector<ustl::vector<ArchmemIPT*>> unmapped_shared_pages_;

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
   * , called by archmem copy constructor
   * The parent should already be somewhere in the vector, and since the child should also mapped to the same shared page as the parent, it should 
   * also be inserted into the same vector. This func search which vector the parents belong to, then add the child to the same vector
   * @param parent_arch the  archmem of the parent, should already exists in some sub-vector in unmapped_shared_pages_
   * @param parent_vpn the vpn of the parent archmem that should be mapped to the future shared ppn
   * @param second_arch the child archmem, should not already be in the vector.
   * @param second_vpn the vpn of the child archmem that should be mapped to the same future shared ppn
  */
  void insertPairedUSP(ArchMemory* parent_arch, size_t parent_vpn, ArchMemory* child_arch, size_t child_vpn);

  /**
   * inserting entry to unmapped_shared_pages_. this is for when the shared page is just newly created and currently only used by 1 archmem
   * Since the page is not allocated a ppn yet (not until a PF happens), therefore the page is not yet in the IPT table. Thats why this unmapped_shared_pages_ exists.
  */
  void insertUspEntry(ArchMemory* archmem, size_t vpn);

  /**
   * remove an entry from the unmapped_shared_pages_. This is called when the sarchmem sharing a page is finally allocated a ppn for that page
   * @param archmem the archmem of one of the sharing archmems
   * @param vpn the vpn of that archmem that points to the shared page
  */
  ustl::vector<ArchmemIPT*>& getUspSubVector(ArchMemory* arch_memory, size_t vpn);

  /**
   * similar to getUspSubVector, but removes the sub-vector from the vector unmapped_shared_pages_
  */
  void deleteUspSubVector(ustl::vector<ArchmemIPT*>& sub_vector);

  private:

    int pages_in_ram_ = 0;  //TODOs: not used at the moment
    int pages_on_disk_ = 0;  //TODOs: not used at the moment
};
