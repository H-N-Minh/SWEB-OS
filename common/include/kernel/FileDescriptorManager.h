#pragma once

#include "uvector.h"
#include "uutility.h"
#include "Pipe.h"
#include "LocalFileDescriptor.h"
#include "FileDescriptor.h"
#include "debug.h"

enum class DescriptorFlag{
  FREE = 0,
  BUSY = 1
};

class FileDescriptorEntry {
public:
  int descriptor;
  void* associatedObject;
  DescriptorFlag flags;

  FileDescriptorEntry(int desc, void* obj, int flgs)
      : descriptor(desc), associatedObject(obj), flags(static_cast<DescriptorFlag>(flgs)) {}
};

class FileDescriptorManager {
public:

  static FileDescriptorManager& getInstance();

  int allocateDescriptor(void* associatedObject, int flags);


  void* getAssociatedObject(int fileDescriptor);
  Pipe* getAssociatedPipe(int fileDescriptor);

  void freeDescriptor(int fileDescriptor);

  FileDescriptorManager(FileDescriptorManager const&) = delete;

  void operator=(FileDescriptorManager const&) = delete;

private:

  FileDescriptorManager();

  ~FileDescriptorManager();

  static FileDescriptorManager instance;
  ustl::vector<FileDescriptorEntry> descriptors;
  int nextDescriptor;

  int findFreeDescriptor();
};
