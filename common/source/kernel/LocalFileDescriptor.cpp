#include "LocalFileDescriptor.h"
#include "LocalFileDescriptorTable.h"
#include "debug.h"


LocalFileDescriptor::LocalFileDescriptor(FileDescriptor *global_fd, uint32_t mode, size_t offset, size_t local_fd_id)
    : global_fd_(global_fd), mode_(mode), offset_(offset), localFD_(local_fd_id) {
  global_fd_->incrementRefCount();
  debug(FILEDESCRIPTOR, "Created LocalFileDescriptor with local FD: %zu, global FD: %d, mode: %u, offset: %zu\n", localFD_, global_fd_->getFd(), mode_, offset_);
}

//COPY CONSTUCTOR
LocalFileDescriptor::LocalFileDescriptor(const LocalFileDescriptor &other) noexcept
    : global_fd_(other.global_fd_),
      mode_(other.mode_),
      offset_(other.offset_),
      localFD_(other.localFD_)
{
  debug(FILEDESCRIPTOR, "Copied LocalFileDescriptor with local FD: %zu, global FD: %d, mode: %u, offset: %zu\n",
        localFD_, global_fd_->getFd(), mode_, offset_);
}


LocalFileDescriptor::~LocalFileDescriptor() {
  global_fd_->decrementRefCount();
  debug(FILEDESCRIPTOR, "Deleted LocalFileDescriptor with local FD: %zu, global FD: %d\n", localFD_, global_fd_->getFd());
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

