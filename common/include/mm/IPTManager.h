#pragma once

#include "types.h"
#include "umap.h"
#include "umemory.h"
#include "umultimap.h"
#include "ArchMemory.h"

class IPTEntry {
public:
  int ppn;
  size_t vpn;
  ArchMemory* archmem;

  IPTEntry(int ppn, size_t vpn, ArchMemory* archmem) : ppn(ppn), vpn(vpn), archmem(archmem) {}
};

typedef int ppn_t;
typedef int diskoffset_t;

class IPTManager {
public:
  ustl::multimap<ppn_t, ustl::shared_ptr<IPTEntry>> ramMap;
  ustl::map<diskoffset_t, ustl::shared_ptr<IPTEntry>> diskMap;

  void addEntryToRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  void addEntryToDisk(diskoffset_t diskOffset, size_t vpn, ArchMemory* archmem);
  ustl::shared_ptr<IPTEntry> lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  ustl::shared_ptr<IPTEntry> lookupEntryInDisk(diskoffset_t diskOffset);
  void removeEntryFromRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem);
  void swapOutPage(ppn_t ppn, diskoffset_t diskOffset);
  void swapInPage(diskoffset_t diskOffset, ppn_t ppn);
};
