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
  bool isLockedByUs() const;

};


class IPTEntry
{
private:
  // use Getter func instead
  ustl::vector<ArchmemIPT*> archmemIPTs_;
  bool pre_swapped_{};
  size_t disk_offset_{};

public:
  // for PRA, keep track of how many times this PPN is accessed
  uint32 access_counter_;


  IPTEntry();
  ~IPTEntry();


  /**
   * check if the given searchArchmem is part of the vector
  */
  bool isArchmemExist(ArchMemory*searchArchmem);

  /**
   * check if the vector is empty
  */
  bool isEmpty();

  void addArchmemIPT(size_t vpn, ArchMemory* archmem);

  void removeArchmemIPT(size_t vpn, ArchMemory* archmem);

  ustl::vector<ArchmemIPT*>& getArchmemIPTs();

  void setPreSwapped(size_t disk_offset);
  bool isPreSwapped() const;
  size_t getDiskOffset() const;
};