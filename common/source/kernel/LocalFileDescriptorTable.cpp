#include "LocalFileDescriptorTable.h"

/**
 * @class LocalFileDescriptorTable
 * @brief Represents a table of local file descriptors.
 *
 * This class manages a table of local file descriptors. It provides functionality for creating, retrieving,
 * and closing local file descriptors. Each local file descriptor corresponds to a global file descriptor.
 */
LocalFileDescriptorTable::LocalFileDescriptorTable() = default;

/**
 * @class LocalFileDescriptorTable
 * @brief Manages local file descriptors and their associated global file descriptors.
 *
 * The LocalFileDescriptorTable class is responsible for managing local file descriptors
 * and their associated global file descriptors. It provides methods to create, retrieve,
 * close, and clear local file descriptors.
 */
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

/**
 * @class LocalFileDescriptorTable
 * @brief The LocalFileDescriptorTable class represents a table that manages local file descriptors.
 */
LocalFileDescriptor* LocalFileDescriptorTable::createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset)
{
  size_t local_fd_id = generateLocalFD();

  global_fd->incrementRefCount();

  auto* local_fd = new LocalFileDescriptor(global_fd, mode, offset, local_fd_id);
  local_fds_.push_back(local_fd);
  return local_fd;
}

/**
 * @brief Retrieves a LocalFileDescriptor with the given local file descriptor ID.
 *
 * This function searches for a LocalFileDescriptor in the local file descriptor table with the specified local file descriptor ID.
 * If found, it returns a pointer to the LocalFileDescriptor. If not found, it returns nullptr.
 *
 * @param local_fd_id The local file descriptor ID to search for.
 * @return A pointer to the LocalFileDescriptor with the specified ID, or nullptr if not found.
 */
[[maybe_unused]] LocalFileDescriptor* LocalFileDescriptorTable::getLocalFileDescriptor(int local_fd_id) const
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

/**
 * @class LocalFileDescriptorTable
 * @brief A class representing a table for managing local file descriptors.
 *
 * This class provides functionality to generate local file descriptors and manage
 * them in a table. It keeps track of the next available file descriptor and provides
 * methods to create, retrieve, and close file descriptors.
 */
size_t LocalFileDescriptorTable::generateLocalFD()
{
  static size_t next_fd = 0;
  return next_fd++;
}

//void LocalFileDescriptorTable::closeLocalFileDescriptor(LocalFileDescriptor *local_fd) {
//  local_fd = nullptr;
//}

/**
 * @brief Close all file descriptors in the local file descriptor table.
 *
 * This function iterates over all file descriptors in the local file descriptor table and closes them.
 * If the global file descriptor associated with a local file descriptor has a reference count of 0,
 * it is deleted. Finally, each local file descriptor is deleted, and the local file descriptor table is cleared.
 */
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