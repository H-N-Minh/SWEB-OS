#include "LocalFileDescriptorTable.h"
#include "debug.h"
#include "UserThread.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() : lfds_lock_("Local FDs Lock") {}

LocalFileDescriptorTable::~LocalFileDescriptorTable()
{
  closeAllFileDescriptors();
}

LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset) {
  ScopeLock l(lfds_lock_);
  size_t local_fd_id = generateLocalFD();
  auto* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  global_fd->incrementRefCount3();
  debug(FILEDESCRIPTOR, "Created and added local file descriptor with local FD ID: %zu. Current count: %d\n", local_fd_id, global_fd->getRefCount3());
  debug(FILEDESCRIPTOR, "After assignment in createLocalFileDescriptor: Global FD: %d\n", local_fd->getGlobalFileDescriptor()->getFd());
  debug(Fabi, "createLocalFileDescriptor:: Global FD = %u; Local FD = %zu; RefCount2 = %d\n;", global_fd->getFd(), local_fd->getLocalFD(), global_fd->getRefCount3());

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
  while (!local_fds_.empty()) {
    removeLocalFileDescriptorUnlocked(local_fds_.back());
  }
  debug(FILEDESCRIPTOR, "Closed all local file descriptors.\n");
}

void LocalFileDescriptorTable::removeLocalFileDescriptor(LocalFileDescriptor* local_fd) {
  ScopeLock l(lfds_lock_);
  removeLocalFileDescriptorUnlocked(local_fd);
}

void LocalFileDescriptorTable::removeLocalFileDescriptorUnlocked(LocalFileDescriptor* local_fd) {
  auto it = ustl::find(local_fds_.begin(), local_fds_.end(), local_fd);

  if (it != local_fds_.end()) {
    FileDescriptor* global_fd = local_fd->getGlobalFileDescriptor(); //here the global fd somehow gets 0;

    debug(FILEDESCRIPTOR, "Before decrement in removeLocalFileDescriptorUnlocked: Global FD: %d\n", global_fd->getFd());

    debug(FILEDESCRIPTOR, "Decrementing ref count for global FD %d\n", global_fd->getFd());

    global_fd->decrementRefCount3();
    //assert(global_fd->getRefCount3() < 0 && "Reference count is negative!");

    if (global_fd->getRefCount3() == 0) {
      debug(FILEDESCRIPTOR, "Ref count is 0 for global FD %d. Deleting it.\n",
            global_fd->getFd());
      delete global_fd;
      if (global_fd_list.remove(global_fd) == -1) {
        debug(FILEDESCRIPTOR, "Failed to remove global FD %d.\n", global_fd->getFd());
      }
    }


    debug(FILEDESCRIPTOR, "Removing local file descriptor: %zu\n", local_fd->getLocalFD());
    local_fds_.erase(it);

    debug(Fabi, "removeLocalFileDescriptor:: Global FD = %u; Local FD = %zu; RefCount2 = %d\n;", global_fd->getFd(), local_fd->getLocalFD(), global_fd->getRefCount3());
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