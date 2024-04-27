#pragma once

#include "uvector.h"
#include "LocalFileDescriptor.h"

class LocalFileDescriptorTable {
public:
  LocalFileDescriptorTable();
  ~LocalFileDescriptorTable();

  LocalFileDescriptor* createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset);

  LocalFileDescriptor* getLocalFileDescriptor(int local_fd_id) const;


  void closeAllFileDescriptors();
  void removeLocalFileDescriptor(LocalFileDescriptor* local_fd);
  static void deleteGlobalFileDescriptor(FileDescriptor* global_fd);
  void removeLocalFileDescriptorUnlocked(LocalFileDescriptor* local_fd);
  ustl::vector<LocalFileDescriptor*> getLocalFileDescriptors() const;
  void addLocalFileDescriptor(LocalFileDescriptor* local_fd);

private:
  ustl::vector<LocalFileDescriptor*> local_fds_;
  static size_t generateLocalFD();
  mutable Mutex lfds_lock_;
};
