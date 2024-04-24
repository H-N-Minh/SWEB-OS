#include "LocalFileDescriptorTable.h"
#include "debug.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() : lfds_lock_("Local FDs Lock") {}

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

LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset) {
  ScopeLock l(lfds_lock_);
  size_t local_fd_id = generateLocalFD();
  global_fd->incrementRefCount();
  auto* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  debug(FILEDESCRIPTOR, "Created and added local file descriptor with local FD ID: %zu\n", local_fd_id);
  return local_fd;
}


LocalFileDescriptor* LocalFileDescriptorTable::getLocalFileDescriptor(int local_fd_id) const
{
  ScopeLock l(lfds_lock_);
  for(auto &fd : local_fds_)
  {
    if(fd->getLocalFD() == (size_t)local_fd_id)
    {
      debug(FILEDESCRIPTOR, "Getting local file descriptor with local FD ID: %d\n", local_fd_id);
      return fd;
    }
  }

  return nullptr;
}

size_t LocalFileDescriptorTable::generateLocalFD()
{
  static size_t next_fd = 3;
  return next_fd++;
}

void LocalFileDescriptorTable::closeAllFileDescriptors() {
  ScopeLock l(lfds_lock_);
  for (auto &fd: local_fds_) {
    fd->getGlobalFileDescriptor()->decrementRefCount();

    if (fd->getGlobalFileDescriptor()->getRefCount() == 0) {
      if (fd->getGlobalFileDescriptor()->getRefCount() == 0) {
        delete fd->getGlobalFileDescriptor();
      }

      delete fd;
    }
    debug(FILEDESCRIPTOR, "Closing all local file descriptors.\n");
    local_fds_.clear();
  }
}

void LocalFileDescriptorTable::removeLocalFileDescriptor(LocalFileDescriptor* local_fd) {
  ScopeLock l(lfds_lock_);
  auto it = ustl::find(local_fds_.begin(), local_fds_.end(), local_fd);
  if (it != local_fds_.end()) {
    FileDescriptor* global_fd = local_fd->getGlobalFileDescriptor();
    global_fd->decrementRefCount();

    if (global_fd->getRefCount() == 0) {
      delete global_fd;
    }
    debug(FILEDESCRIPTOR, "Removing local file descriptor: %zu\n", local_fd->getLocalFD());
    local_fds_.erase(it);
    delete local_fd;
  }
}

ustl::vector<LocalFileDescriptor*> LocalFileDescriptorTable::getLocalFileDescriptors() const
{
  ScopeLock l(lfds_lock_);
  return local_fds_;
}

void LocalFileDescriptorTable::addLocalFileDescriptor(LocalFileDescriptor* local_fd)
{
  ScopeLock l(lfds_lock_);
  local_fds_.push_back(local_fd);
  debug(FILEDESCRIPTOR, "Adding local file descriptor: %zu\n", local_fd->getLocalFD());
}