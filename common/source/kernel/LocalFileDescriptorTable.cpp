#include "LocalFileDescriptorTable.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() {
}

LocalFileDescriptorTable::~LocalFileDescriptorTable() {
  for (auto fd : local_fds_) {
    delete fd;
  }
  local_fds_.clear();
}

LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset) {
  int local_fd_id = generateLocalFD();
  LocalFileDescriptor* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  return local_fd;
}

//void LocalFileDescriptorTable::closeLocalFileDescriptor(LocalFileDescriptor* local_fd) {
//  if (local_fd) {
//    FileDescriptor* global_fd = local_fd->getGlobalFileDescriptor();
//
//    global_fd->decrementRefCount();
//
//    if (global_fd->getRefCount() == 0) {
//      global_fd_list.remove(global_fd->getFd());
//
//      delete global_fd;
//    }
//
//    auto it = ustl::find(local_fds_.begin(), local_fds_.end(), local_fd);
//    if (it != local_fds_.end()) {
//      local_fds_.erase(it);
//    }
//
//    delete local_fd;
//  }
//}


LocalFileDescriptor* LocalFileDescriptorTable::getLocalFileDescriptor(int local_fd_id) const {
  for (auto fd : local_fds_) {
    if (fd->getLocalFD() == (size_t)local_fd_id) {
      return fd;
    }
  }
  return nullptr;
}

//void LocalFileDescriptorTable::closeAllFileDescriptors() {
//  for (auto fd : local_fds_) {
//    closeLocalFileDescriptor(fd);
//  }
//}

size_t LocalFileDescriptorTable::generateLocalFD() {
  static int next_fd = 0;
  return next_fd++;
}
