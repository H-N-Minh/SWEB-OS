#pragma once

#include "FileDescriptor.h"
#include "FileType.h"

class LocalFileDescriptor {
public:
  LocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset, size_t local_fd_id, FileType type);
  LocalFileDescriptor(const LocalFileDescriptor &other) noexcept;
  ~LocalFileDescriptor();

  FileDescriptor* getGlobalFileDescriptor() const;

  uint32_t getMode() const;

  void setOffset(size_t offset);

  size_t getOffset() const;
  size_t getLocalFD() const;

  FileType getType() const;


private:
  FileDescriptor* global_fd_;
  uint32_t mode_;
  size_t offset_;
  size_t localFD_;
  FileType type_;

};