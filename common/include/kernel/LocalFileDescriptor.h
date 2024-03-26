#pragma once

#include "FileDescriptor.h"

class LocalFileDescriptor {
public:
  LocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset, size_t local_fd_id);
  ~LocalFileDescriptor();

  FileDescriptor* getGlobalFileDescriptor() const;

  [[maybe_unused]] uint32_t getMode() const;

  [[maybe_unused]] void setOffset(size_t offset);

  [[maybe_unused]] size_t getOffset() const;
  size_t getLocalFD() const;


private:
  FileDescriptor* global_fd_;
  uint32_t mode_;
  size_t offset_;
  size_t localFD_;
};
