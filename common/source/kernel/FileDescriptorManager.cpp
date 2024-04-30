#include "FileDescriptorManager.h"
#include "debug.h"

FileDescriptorManager FileDescriptorManager::instance;

FileDescriptorManager& FileDescriptorManager::getInstance() {
  return instance;
}

FileDescriptorManager::FileDescriptorManager() : nextDescriptor(3) {}

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
    debug(PIPE, "FileDescriptorManager::allocateDescriptor: associatedObject is nullptr\n");
    return -1;
  }

  debug(PIPE, "FileDescriptorManager::allocateDescriptor called\n");
  debug(PIPE, "Descriptor vector size: %lu\n", descriptors.size());

  int descriptor = findFreeDescriptor();
  if (descriptor < 0) {
    debug(PIPE, "FileDescriptorManager::allocateDescriptor negative descriptor returned from findFreeDescriptor\n");
    return -1;
  }

  descriptors.push_back(FileDescriptorEntry(descriptor, associatedObject, flags));

  debug(PIPE, "FileDescriptorManager::allocateDescriptor allocated descriptor: %d\n", descriptor);

  return descriptor;
}

void* FileDescriptorManager::getAssociatedObject(int fileDescriptor) {
  debug(PIPE, "FileDescriptorManager::getAssociatedObject: Searching for fileDescriptor: %d\n", fileDescriptor);

  for (auto& entry : descriptors) {
    if (entry.descriptor == fileDescriptor) {
      return entry.associatedObject;
    }
  }

  debug(PIPE, "FileDescriptorManager::getAssociatedObject: fileDescriptor not found: %d\n", fileDescriptor);
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
  debug(PIPE, "FileDescriptorManager::freeDescriptor: Trying to free fileDescriptor: %d\n", fileDescriptor);

  for (auto & descriptor : descriptors) {
    if (descriptor.descriptor == fileDescriptor) {
      descriptor.flags = DescriptorFlag::FREE;
      return;
    }
  }
  debug(PIPE, "FileDescriptorManager::freeDescriptor: fileDescriptor not found: %d\n", fileDescriptor);
}

int FileDescriptorManager::findFreeDescriptor() {
  debug(PIPE, "FileDescriptorManager::findFreeDescriptor: nextDescriptor before increment: %d\n", nextDescriptor);

  int nextDescInt = static_cast<int>(nextDescriptor);
  assert(nextDescInt >= 0 && "Next descriptor value overflowed!");

  int free_descriptor = nextDescriptor++;

  debug(PIPE, "FileDescriptorManager::findFreeDescriptor: nextDescriptor after increment: %d\n", nextDescriptor);

  return free_descriptor;
}























































































































































































































































