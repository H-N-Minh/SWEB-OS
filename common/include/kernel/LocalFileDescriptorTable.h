#pragma once

#include "uvector.h"
#include "LocalFileDescriptor.h"

class LocalFileDescriptorTable {
public:
  LocalFileDescriptorTable();
  ~LocalFileDescriptorTable();

  LocalFileDescriptor* createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset);
  void closeLocalFileDescriptor(LocalFileDescriptor* local_fd);
  LocalFileDescriptor* getLocalFileDescriptor(int local_fd_id) const;
  void closeAllFileDescriptors();


private:
  ustl::vector<LocalFileDescriptor*> local_fds_;
  size_t generateLocalFD();
};
