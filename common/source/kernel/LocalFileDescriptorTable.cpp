#include "LocalFileDescriptorTable.h"
#include "debug.h"
#include "UserThread.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() : lfds_lock_("Local FDs Lock") {}

LocalFileDescriptorTable::~LocalFileDescriptorTable()
{
  closeAllFileDescriptors();
}

LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset) {
  size_t local_fd_id = generateLocalFD();
  auto* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  global_fd->incrementRefCount();
  debug(FILEDESCRIPTOR, "Created and added local file descriptor with local FD ID: %zu. Current count: %d\n", local_fd_id, global_fd->getRefCount());
  debug(FILEDESCRIPTOR, "After assignment in createLocalFileDescriptor: Global FD: %d\n", local_fd->getGlobalFileDescriptor()->getFd());
  debug(Fabi, "createLocalFileDescriptor:: Global FD = %u; Local FD = %zu; RefCount2 = %d\n;", global_fd->getFd(), local_fd->getLocalFD(), global_fd->getRefCount());

  return local_fd;
}


LocalFileDescriptor* LocalFileDescriptorTable::getLocalFileDescriptor(int local_fd_id) const
{
  assert(lfds_lock_.heldBy() == currentThread);
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
  assert(lfds_lock_.heldBy() != currentThread);
  ScopeLock l(lfds_lock_);
  while (!local_fds_.empty()) {
    removeLocalFileDescriptor(local_fds_.back());
  }
  debug(FILEDESCRIPTOR, "Closed all local file descriptors.\n");
}


void LocalFileDescriptorTable::deleteGlobalFileDescriptor(FileDescriptor* global_fd)
{
  debug(FILEDESCRIPTOR, "Ref count is 0 for global FD %d. Deleting it.\n", global_fd->getFd());
  if (global_fd_list.remove(global_fd) == -1)
  {
    debug(FILEDESCRIPTOR, "Failed to remove the global FD %d.\n", global_fd->getFd());
  }
  delete global_fd;
  global_fd = nullptr;
}

void LocalFileDescriptorTable::removeLocalFileDescriptor(LocalFileDescriptor* local_fd) {
  assert(lfds_lock_.heldBy() == currentThread);
  auto it = ustl::find(local_fds_.begin(), local_fds_.end(), local_fd);
  if (it != local_fds_.end()) {
    FileDescriptor* global_fd = local_fd->getGlobalFileDescriptor();
    debug(FILEDESCRIPTOR, "Before decrement in removeLocalFileDescriptorUnlocked: Global FD: %d\n", global_fd->getFd());
    debug(FILEDESCRIPTOR, "Decrementing ref count for global FD %d\n", global_fd->getFd());
    global_fd->decrementRefCount();

    if (global_fd->getRefCount() == 0) {
      deleteGlobalFileDescriptor(global_fd);
    }
    debug(FILEDESCRIPTOR, "Removing local file descriptor: %zu\n", local_fd->getLocalFD());
    local_fds_.erase(it);
    delete local_fd;
  }
}

ustl::vector<LocalFileDescriptor*> LocalFileDescriptorTable::getLocalFileDescriptors() const
{
  assert(lfds_lock_.heldBy() == currentThread);
  return local_fds_;
}

void LocalFileDescriptorTable::addLocalFileDescriptor(LocalFileDescriptor* local_fd)
{
  assert(lfds_lock_.heldBy() == currentThread);
  local_fds_.push_back(local_fd);
  debug(FILEDESCRIPTOR, "Adding local file descriptor: %zu\n", local_fd->getLocalFD());
}