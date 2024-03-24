#include "FileDescriptorManager.h"



FileDescriptorManager FileDescriptorManager::instance;

/**
 * @class FileDescriptorManager
 * @brief Manages file descriptors and their associated objects
 */
FileDescriptorManager& FileDescriptorManager::getInstance() {
  return instance;
}

/**
 * @brief Constructor for the FileDescriptorManager class
 *
 * This constructor initializes a FileDescriptorManager object.
 * It sets the initial value of the nextDescriptor member variable to 3.
 * The initial value of 3 is chosen because file descriptors 0, 1, and 2 are typically reserved for stdin, stdout, and stderr.
 */
FileDescriptorManager::FileDescriptorManager() : nextDescriptor(3) {
  //starts with 3 because others for stdin etc.
}

/**
 * @brief FileDescriptorManager destructor
 *
 * The destructor for the FileDescriptorManager class.
 *
 * It has a default implementation, which means it does not perform any specific cleanup tasks.
 *
 * @remark This function is automatically called when the FileDescriptorManager object is destroyed.
 */
FileDescriptorManager::~FileDescriptorManager() = default;

/**
 * @brief Allocates a file descriptor and associates it with an object
 *
 * This function is used to allocate a file descriptor and associate it with an object. The file descriptor
 * can be used to identify and access the associated object later. The associated object and flags are provided
 * as input parameters.
 *
 * @param associatedObject - A pointer to the object to be associated with the file descriptor
 * @param flags - An integer value representing any flags or attributes associated with the file descriptor
 *
 * @return An integer value representing the allocated file descriptor. Returns -1 if no free descriptors are available.
 *
 * @remark The associated object can be retrieved later using the `getAssociatedObject()` function.
 */
int FileDescriptorManager::allocateDescriptor(void* associatedObject, int flags) {
  int descriptor = findFreeDescriptor();
  if (descriptor != -1) {
    descriptors.push_back(FileDescriptorEntry(descriptor, associatedObject, flags));
  }
  return descriptor;
}

/**
   * @brief Returns the associated object of a file descriptor
   *
   * This function searches for the given file descriptor in the descriptors vector and retrieves its associated object.
   *
   * @param fileDescriptor - The file descriptor for which to retrieve the associated object
   *
   * @return A void pointer to the associated object if found, otherwise nullptr
   *
   * @remark The associated object can be set using the `allocateDescriptor()` function. If the file descriptor is not found, nullptr is returned.
   */
[[maybe_unused]] void* FileDescriptorManager::getAssociatedObject(int fileDescriptor) {
  for (auto& entry : descriptors) {
    if (entry.descriptor == fileDescriptor) {
      return entry.associatedObject;
    }
  }
  return nullptr;
}

/**********************************
 * @fn void FileDescriptorManager::freeDescriptor(int fileDescriptor)
 * @brief Frees a file descriptor and removes it from the manager.
 *
 * This function frees the given file descriptor and removes it from the manager.
 * The corresponding FileDescriptorEntry object associated with the file descriptor will be erased from the descriptors vector.
 * If the file descriptor is not found in the descriptors vector, no action will be taken.
 *
 * @param fileDescriptor The file descriptor to be freed.
 **********************************/
[[maybe_unused]] void FileDescriptorManager::freeDescriptor(int fileDescriptor) {
  for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
    if (it->descriptor == fileDescriptor) {
      descriptors.erase(it);
      break;
    }
  }
}

/**
 * @brief Finds the next available file descriptor
 *
 * This function finds the next available file descriptor. It increments the value of
 * `nextDescriptor` and returns it.
 *
 * @return An integer value representing the next available file descriptor.
 */
int FileDescriptorManager::findFreeDescriptor() {
//probably needs a better mechanism -> this is basic
  return nextDescriptor++;
}


