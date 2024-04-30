#include "LocalFileDescriptor.h"
#include "LocalFileDescriptorTable.h"


LocalFileDescriptor::LocalFileDescriptor(FileDescriptor *global_fd, uint32_t mode, size_t offset, size_t local_fd_id)
    : global_fd_(global_fd), mode_(mode), offset_(offset), localFD_(local_fd_id) {
  global_fd_->incrementRefCount();
}


LocalFileDescriptor::~LocalFileDescriptor() {
  global_fd_->decrementRefCount();
}


FileDescriptor *LocalFileDescriptor::getGlobalFileDescriptor() const {
  return global_fd_;
}

uint32_t LocalFileDescriptor::getMode() const {
  return mode_;
}

void LocalFileDescriptor::setOffset(size_t offset) {
  offset_ = offset;
}

size_t LocalFileDescriptor::getOffset() const {
  return offset_;
}

size_t LocalFileDescriptor::getLocalFD() const {
  return localFD_;
}

