#include "FileDescriptorManager.h"
#include "debug.h"

FileDescriptorManager FileDescriptorManager::instance;

FileDescriptorManager& FileDescriptorManager::getInstance() {
  return instance;
}


FileDescriptorManager::FileDescriptorManager() : nextDescriptor(3) {
  descriptors.reserve(4096);
}

FileDescriptorManager::~FileDescriptorManager() = default;


int FileDescriptorManager::allocateDescriptor(void* associatedObject, int flags) {


  for (auto& entry : descriptors) {
    if (entry.flags == DescriptorFlag::FREE) {
      entry.associatedObject = associatedObject;
      entry.flags = DescriptorFlag::BUSY;
      return entry.descriptor;
    }
  }

  if (associatedObject == nullptr) {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor: associatedObject is nullptr\n");
    return -1;
  }

  if (descriptors.size() >= descriptors.capacity()) {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor: descriptor vector is full, increasing capacity\n");
    if (descriptors.capacity() == 0) {
      descriptors.reserve(1);
    } else {
      while (descriptors.size() >= descriptors.capacity()) {
        descriptors.reserve(descriptors.capacity() * 2);
      }
    }
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor called\n");
  debug(FILEDESCRIPTOR, "Descriptor vector size: %lu, Capacity: %lu\n", descriptors.size(), descriptors.capacity());
  int descriptor = findFreeDescriptor();
  if (descriptor < 0) {
    debug(Fabi, "FileDescriptorManager::allocateDescriptor negative descriptor returned from findFreeDescriptor\n");
    return -1;
  }

  if (descriptors.size() < descriptors.capacity()) {
    descriptors.push_back(FileDescriptorEntry(descriptor, associatedObject, flags));
  } else {
    debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor descriptor vector is full\n");
    return -1;
  }

  debug(FILEDESCRIPTOR, "FileDescriptorManager::allocateDescriptor allocated descriptor: %d\n", descriptor);

  return descriptor;
}

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


Pipe* FileDescriptorManager::getAssociatedPipe(int fileDescriptor) {
  void* associatedObject = getAssociatedObject(fileDescriptor);

  if (associatedObject == nullptr) {
    return nullptr;
  }

  Pipe* pipe = static_cast<Pipe*>(associatedObject);
  return pipe;
}

void FileDescriptorManager::freeDescriptor(int fileDescriptor) {
  debug(Fabi, "FileDescriptorManager::freeDescriptor: Trying to free fileDescriptor: %d\n", fileDescriptor);

  for (auto & descriptor : descriptors) {
    if (descriptor.descriptor == fileDescriptor) {
      descriptor.flags = DescriptorFlag::FREE;
      return;
    }
  }
  debug(FILEDESCRIPTOR, "FileDescriptorManager::freeDescriptor: fileDescriptor not found: %d\n", fileDescriptor);
}

int FileDescriptorManager::findFreeDescriptor() {
  debug(Fabi, "FileDescriptorManager::findFreeDescriptor: nextDescriptor before increment: %d\n", nextDescriptor);

  int nextDescInt = static_cast<int>(nextDescriptor);
  assert(nextDescInt >= 0 && "Next descriptor value overflowed!");

  int free_descriptor = nextDescriptor++;

  debug(Fabi, "FileDescriptorManager::findFreeDescriptor: nextDescriptor after increment: %d\n", nextDescriptor);

  return free_descriptor;
}


