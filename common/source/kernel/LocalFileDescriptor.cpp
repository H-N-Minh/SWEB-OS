#include "LocalFileDescriptor.h"

LocalFileDescriptor::LocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset)
    : global_fd_(global_fd), mode_(mode), offset_(offset) {
}

LocalFileDescriptor::~LocalFileDescriptor() {
}

FileDescriptor* LocalFileDescriptor::getGlobalFileDescriptor() const {
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
