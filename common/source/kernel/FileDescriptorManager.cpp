#include "FileDescriptorManager.h"
#include "debug.h"

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
  descriptors.reserve(4096);
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

  if (associatedObject == nullptr) {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor: associatedObject is nullptr\n");
    return -1;


  }

  if (descriptors.size() >= descriptors.capacity()) {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor: descriptor vector is full, increasing capacity\n");
    if (descriptors.capacity() == 0) {
      descriptors.reserve(1);
    } else {
      descriptors.reserve(descriptors.capacity() * 2);
    }
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor called\n");
  debug(FILEDESCRIPTOR, "Descriptor vector size: %lu, Capacity: %lu\n", descriptors.size(), descriptors.capacity());
  int descriptor = findFreeDescriptor();
  if (descriptor < 0) {
    debug(Fabi, "FileDescriptorManager::allocateDescriptor negative descriptor returned from findFreeDescriptor\n");
    return -1;
  }

  // Check available capacity before push_back
  if (descriptors.size() < descriptors.capacity()) {
    descriptors.push_back(FileDescriptorEntry(descriptor, associatedObject, flags));
  } else {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor descriptor vector is full\n");
    return -1;
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor allocated descriptor: %d\n", descriptor);

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
void* FileDescriptorManager::getAssociatedObject(int fileDescriptor) {
  debug(FILEDESCRIPTOR, "FileDescriptorManager::getAssociatedObject: Searching for fileDescriptor: %d\n", fileDescriptor);

  for (auto& entry : descriptors) {
    if (entry.descriptor == fileDescriptor) {
      return entry.associatedObject;
    }
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::getAssociatedObject: fileDescriptor not found: %d\n", fileDescriptor);
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
void FileDescriptorManager::freeDescriptor(int fileDescriptor) {
  debug(Fabi, "FileDescriptorManager::freeDescriptor: Trying to free fileDescriptor: %d\n", fileDescriptor);

  for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
    if (it->descriptor == fileDescriptor) {
      descriptors.erase(it);
      return;
    }
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::freeDescriptor: fileDescriptor not found: %d\n", fileDescriptor);
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
  debug(Fabi, "FileDescriptorManager::findFreeDescriptor: nextDescriptor before increment: %d\n", nextDescriptor);

  int nextDescInt = static_cast<int>(nextDescriptor);
  assert(nextDescInt >= 0 && "Next descriptor value overflowed!");

  int free_descriptor = nextDescriptor++;

  debug(Fabi, "FileDescriptorManager::findFreeDescriptor: nextDescriptor after increment: %d\n", nextDescriptor);

  return free_descriptor;
}


