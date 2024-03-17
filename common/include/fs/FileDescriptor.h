#pragma once

#include "types.h"
#include "ulist.h"
#include "umap.h"
#include "Mutex.h"

class File;
class FileDescriptor;
class FileDescriptorList;

class FileDescriptor
{
protected:
  size_t fd_;
  File* file_;

public:
  FileDescriptor ( File* file );
  virtual ~FileDescriptor();
  uint32 getFd() { return fd_; }
  File* getFile() { return file_; }

  friend File;
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


///////// neue version die noch nicht geht
//#pragma once
//
//#include "types.h"
//#include "ulist.h"
//#include "umap.h"
//#include "Mutex.h"
//
//class File;
//class FileDescriptor;
//class FileDescriptorList;
//
//class FileDescriptor
//{
//protected:
//  size_t fd_;
//  File* file_;
//  int ref_count_;
//
//public:
//  FileDescriptor ( File* file );
//  virtual ~FileDescriptor();
//  uint32 getFd() const { return fd_; }
//  File* getFile() const { return file_; }
//
//  friend File;
//  void incrementRefCount();
//  void decrementRefCount();
//  int getRefCount() const;
//};
//
//class FileDescriptorList
//{
//public:
//  FileDescriptorList();
//  ~FileDescriptorList();
//
//  int add(FileDescriptor* fd);
//  int remove(FileDescriptor* fd);
//  bool remove(size_t fd_num);
//  FileDescriptor* getFileDescriptor(uint32 fd);
//
//private:
//  ustl::list<FileDescriptor*> fds_;
//  Mutex fd_lock_;
//};
//
//extern FileDescriptorList global_fd_list;