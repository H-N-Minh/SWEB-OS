#pragma once

#include "uvector.h"
#include "LocalFileDescriptor.h"

class LocalFileDescriptorTable {
public:
  LocalFileDescriptorTable();
  ~LocalFileDescriptorTable();

  LocalFileDescriptor* createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset);

  //[[maybe_unused]] static void closeLocalFileDescriptor(LocalFileDescriptor* local_fd);

  [[maybe_unused]] [[nodiscard]] LocalFileDescriptor* getLocalFileDescriptor(int local_fd_id) const;

  [[maybe_unused]] void closeAllFileDescriptors();


private:
  ustl::vector<LocalFileDescriptor*> local_fds_;
  static size_t generateLocalFD();
};
