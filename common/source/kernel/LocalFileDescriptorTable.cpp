#include "LocalFileDescriptorTable.h"

LocalFileDescriptorTable::LocalFileDescriptorTable() {}

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

  LocalFileDescriptor* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
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