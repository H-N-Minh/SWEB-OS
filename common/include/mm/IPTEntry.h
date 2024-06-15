#pragma once

// #include "IPTManager.h"
#include "uvector.h"

class ArchMemory;
class IPTManager;

class ArchmemIPT 
{
public:
  size_t vpn_;
  ArchMemory* archmem_;

  ArchmemIPT(size_t vpn, ArchMemory* archmem);

  /**
   * check if the archmem of this entry is locked
  */
  bool isLockedByUs();

};


class IPTEntry
{
private:
  // use Getter func instead
  ustl::vector<ArchmemIPT*> archmemIPTs_;

public:
  // for PRA, keep track of how many times this PPN is accessed
  uint32 access_counter_;

  size_t last_disk_offset_{0};


  IPTEntry();
  ~IPTEntry();


  /**
   * check if the given searchArchmem is part of the vector
  */
  bool isArchmemExist(ArchMemory* archmem, size_t vpn);

  /**
   * check if the vector is empty
  */
  bool isEmpty();

  void addArchmemIPT(size_t vpn, ArchMemory* archmem);

  void removeArchmemIPT(size_t vpn, ArchMemory* archmem);

  ustl::vector<ArchmemIPT*>& getArchmemIPTs();
};