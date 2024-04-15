#include "LocalFileDescriptorTable.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() = default;

LocalFileDescriptorTable::~LocalFileDescriptorTable()
{
  for(auto &fd : local_fds_)
  {
    fd->getGlobalFileDescriptor()->decrementRefCount();

    if (fd->getGlobalFileDescriptor()->getRefCount() == 0)
    {
      delete fd->getGlobalFileDescriptor();
    }

    delete fd;
  }
  local_fds_.clear();
}

LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset)
{
  size_t local_fd_id = generateLocalFD();

  global_fd->incrementRefCount();

  auto* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  return local_fd;
}


LocalFileDescriptor* LocalFileDescriptorTable::getLocalFileDescriptor(int local_fd_id) const
{
  for(auto &fd : local_fds_)
  {
    if(fd->getLocalFD() == (size_t)local_fd_id)
    {
      return fd;
    }
  }
  return nullptr;
}

size_t LocalFileDescriptorTable::generateLocalFD()
{
  static size_t next_fd = 0;
  return next_fd++;
}

//void LocalFileDescriptorTable::closeLocalFileDescriptor(LocalFileDescriptor *local_fd) {
//  local_fd = nullptr;
//}


void LocalFileDescriptorTable::closeAllFileDescriptors() {
  for (auto &fd: local_fds_) {
    fd->getGlobalFileDescriptor()->decrementRefCount();

    if (fd->getGlobalFileDescriptor()->getRefCount() == 0) {
      if (fd->getGlobalFileDescriptor()->getRefCount() == 0) {
        delete fd->getGlobalFileDescriptor();
      }

      delete fd;
    }

    local_fds_.clear();
  }
}

void LocalFileDescriptorTable::removeLocalFileDescriptor(LocalFileDescriptor* local_fd) {
  auto it = ustl::find(local_fds_.begin(), local_fds_.end(), local_fd);
  if (it != local_fds_.end()) {
    FileDescriptor* global_fd = local_fd->getGlobalFileDescriptor();
    global_fd->decrementRefCount();

    if (global_fd->getRefCount() == 0) {
      delete global_fd;
    }

    local_fds_.erase(it);
    delete local_fd;
  }
}