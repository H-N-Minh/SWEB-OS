#include "LocalFileDescriptor.h"
#include "LocalFileDescriptorTable.h"

/**
 * @brief LocalFileDescriptor class represents a local file descriptor.
 *
 * This class is used to manage local file descriptors that are associated with global file descriptors.
 */
LocalFileDescriptor::LocalFileDescriptor(FileDescriptor *global_fd, uint32_t mode, size_t offset, size_t local_fd_id)
    : global_fd_(global_fd), mode_(mode), offset_(offset), localFD_(local_fd_id) {}


/**
 * @class LocalFileDescriptor
 * @brief Class representing a local file descriptor.
 *
 * The LocalFileDescriptor class is responsible for managing a local file descriptor
 * associated with a global file descriptor. Each local file descriptor keeps a reference
 * to the global file descriptor and decrements its reference count when the
 * local file descriptor is destroyed.
 */
LocalFileDescriptor::~LocalFileDescriptor() {
  global_fd_->decrementRefCount();
}

/**
 * @brief Returns the global file descriptor associated with this local file descriptor.
 *
 * @return A pointer to the FileDescriptor object.
 */
FileDescriptor *LocalFileDescriptor::getGlobalFileDescriptor() const {
  return global_fd_;
}

/**
 * @brief Get the mode of the LocalFileDescriptor.
 *
 * This function returns the mode of the LocalFileDescriptor object.
 *
 * @return The mode of the LocalFileDescriptor as an unsigned 32-bit integer.
 */
uint32_t LocalFileDescriptor::getMode() const {
  return mode_;
}

/**
 * @class LocalFileDescriptor
 * @brief Represents a local file descriptor.
 *
 * The LocalFileDescriptor class is used to manage local file descriptors in a file system.
 * It allows setting and retrieving the offset of the file descriptor.
 */
void LocalFileDescriptor::setOffset(size_t offset) {
  offset_ = offset;
}

/**
 * @brief Get the offset of the LocalFileDescriptor
 *
 * This function returns the offset value of the LocalFileDescriptor object.
 *
 * @return The offset value of the LocalFileDescriptor
 */
size_t LocalFileDescriptor::getOffset() const {
  return offset_;
}

/**
 * Returns the value of the local file descriptor.
 *
 * @return The value of the local file descriptor.
 */
size_t LocalFileDescriptor::getLocalFD() const {
  return localFD_;
}

