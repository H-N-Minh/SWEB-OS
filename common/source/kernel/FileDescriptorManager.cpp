#include "FileDescriptorManager.h"

FileDescriptorManager::FileDescriptorManager() : nextDescriptor(3) {
  //starts with 3 because others for stdin etc.
}

FileDescriptorManager::~FileDescriptorManager() {
}

int FileDescriptorManager::allocateDescriptor(void* associatedObject, int flags) {
  int descriptor = findFreeDescriptor();
  if (descriptor != -1) {
    descriptors.push_back(FileDescriptorEntry(descriptor, associatedObject, flags));
  }
  return descriptor;
}

[[maybe_unused]] void* FileDescriptorManager::getAssociatedObject(int fileDescriptor) {
  for (auto& entry : descriptors) {
    if (entry.descriptor == fileDescriptor) {
      return entry.associatedObject;
    }
  }
  return nullptr;
}

[[maybe_unused]] void FileDescriptorManager::freeDescriptor(int fileDescriptor) {
  for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
    if (it->descriptor == fileDescriptor) {
      descriptors.erase(it);
      break;
    }
  }
}

int FileDescriptorManager::findFreeDescriptor() {
//probably needs a better mechanism -> this is basic
  return nextDescriptor++;
}
