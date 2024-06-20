#pragma once

#include "types.h"
#include "ulist.h"
#include "umap.h"
#include "Mutex.h"
#include "uatomic.h"

class File;
class FileDescriptor;
class FileDescriptorList;

class FileDescriptor
{
protected:
  size_t fd_;
  File* file_;

public:
  enum class FileType {
    REGULAR,
    PIPE,
    SHARED_MEMORY
  };

  explicit FileDescriptor(File* file, FileType type = FileType::REGULAR);
  virtual ~FileDescriptor();
  uint32 getFd() const { return fd_; }
  File* getFile() const { return file_; }

  FileType getType() const { return type_; }

  friend File;
  void incrementRefCount();
  void decrementRefCount();
  int getRefCount() const;

private:
  ustl::atomic<int> ref_count_{0};
  FileType type_;
};

class FileDescriptorList
{
public:
  FileDescriptorList();
  ~FileDescriptorList();

  int add(FileDescriptor* fd);
  int remove(FileDescriptor* fd);

  FileDescriptor* getFileDescriptor(uint32 fd);

private:
  ustl::list<FileDescriptor*> fds_;
  Mutex fd_lock_;
};

extern FileDescriptorList global_fd_list;