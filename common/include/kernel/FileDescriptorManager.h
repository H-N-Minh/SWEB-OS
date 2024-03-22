#pragma once

#include "uvector.h"
#include "uutility.h"
#include "Pipe.h"


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
  static FileDescriptorManager& getInstance() {
    static uint8_t instance[sizeof(FileDescriptorManager)];

    return *new(instance) FileDescriptorManager();
  }

  int allocateDescriptor(void* associatedObject, int flags);

  [[maybe_unused]] void* getAssociatedObject(int fileDescriptor);

  [[maybe_unused]] void freeDescriptor(int fileDescriptor);

  FileDescriptorManager(FileDescriptorManager const&) = delete;
  void operator=(FileDescriptorManager const&) = delete;

private:
  FileDescriptorManager();
  ~FileDescriptorManager();
  ustl::vector<FileDescriptorEntry> descriptors;
  int nextDescriptor;

  int findFreeDescriptor();
};
