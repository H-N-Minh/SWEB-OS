#pragma once

#include "uvector.h"
#include "LocalFileDescriptor.h"
#include "FileType.h"

class LocalFileDescriptorTable {
public:
  static LocalFileDescriptorTable* instance();

  LocalFileDescriptorTable();
  ~LocalFileDescriptorTable();

  LocalFileDescriptor* createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset, FileType type);

  LocalFileDescriptor* getLocalFileDescriptor(int local_fd_id) const;


  void closeAllFileDescriptors();
  void removeLocalFileDescriptor(LocalFileDescriptor* local_fd);


private:
  ustl::vector<LocalFileDescriptor*> local_fds_;
  static size_t generateLocalFD();
  mutable Mutex lfds_lock_;

  static LocalFileDescriptorTable* instance_;
};
