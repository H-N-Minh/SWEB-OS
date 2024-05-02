#pragma once

#include "uvector.h"
#include "LocalFileDescriptor.h"
#include "FileType.h"
#include "debug.h"
#include "Pipe.h"

class LocalFileDescriptorTable {
public:
  static LocalFileDescriptorTable* instance();

  LocalFileDescriptorTable();
  ~LocalFileDescriptorTable();

  LocalFileDescriptor* createLocalFileDescriptor(FileDescriptor* global_fd, uint32_t mode, size_t offset, FileType type);

  LocalFileDescriptor* getLocalFileDescriptor(int local_fd_id) const;


  void closeAllFileDescriptors();
  static void deleteGlobalFileDescriptor(FileDescriptor* global_fd);
  int removeLocalFileDescriptor(LocalFileDescriptor* local_fd);
  ustl::vector<LocalFileDescriptor*> getLocalFileDescriptors() const;
  void addLocalFileDescriptor(LocalFileDescriptor* local_fd);

  mutable Mutex lfds_lock_;

private:
  ustl::vector<LocalFileDescriptor*> local_fds_;
  static size_t generateLocalFD();


  static LocalFileDescriptorTable* instance_;
};
