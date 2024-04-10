#pragma once

#include "uvector.h"
#include "uutility.h"
#include "Pipe.h"
#include "LocalFileDescriptor.h"
#include "FileDescriptor.h"
#include "debug.h"

class FileDescriptorEntry {
public:
  int descriptor;
  void* associatedObject;
  int flags;

  FileDescriptorEntry(int desc, void* obj, int flgs)
      : descriptor(desc), associatedObject(obj), flags(flgs) {}
};

class FileDescriptorManager {
public:

  static FileDescriptorManager& getInstance();

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
  int allocateDescriptor(void* associatedObject, int flags);

  /**
   * @brief Frees a file descriptor and its associated object
   *
   * This function frees the provided file descriptor and its associated object.
   * The file descriptor will be marked as available for reuse.
   *
   * @param fileDescriptor The file descriptor to be freed
   *
   * @note The associated object will not be deleted or released. It is the
   * responsibility of the caller to manage the associated object's lifetime.
   */
  void* getAssociatedObject(int fileDescriptor);

  /**
   * @brief Frees a file descriptor and removes it from the manager.
   *
   * The freeDescriptor function frees the given file descriptor and removes it from the manager.
   * The corresponding FileDescriptorEntry object associated with the file descriptor will be erased from the descriptors vector.
   * If the file descriptor is not found in the descriptors vector, no action will be taken.
   *
   * @param fileDescriptor The file descriptor to be freed.
   *
   * @note This function is not supposed to be called directly, instead, use the FileDescriptorManager class to free file descriptors.
   */
  [[maybe_unused]] void freeDescriptor(int fileDescriptor);

  /**
    * @class FileDescriptorManager
    * @brief Manages file descriptors and their associated objects
    *
    * The FileDescriptorManager class provides functionality to allocate, free, and get the associated object
    * of file descriptors. It is implemented as a singleton, ensuring that only one instance of the class exists.
    *
    * To access the FileDescriptorManager instance, call the static method `getInstance()`.
    */
  FileDescriptorManager(FileDescriptorManager const&) = delete;
  /**
   * @file
   * @brief FileDescriptorManager class documentation
   */
  void operator=(FileDescriptorManager const&) = delete;

private:
  /**
     * @brief Constructs a FileDescriptorManager object
     *
     * The FileDescriptorManager class manages file descriptors and their associated objects.
     * This constructor initializes the FileDescriptorManager object with default values.
     */
  FileDescriptorManager();

  ~FileDescriptorManager();

  static FileDescriptorManager instance;
  ustl::vector<FileDescriptorEntry> descriptors;
  int nextDescriptor;

  int findFreeDescriptor();
};
