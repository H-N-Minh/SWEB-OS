#include "FileDescriptor.h"
#include <ulist.h>
#ifndef EXE2MINIXFS
#include "ArchThreads.h"
#endif
#include "kprintf.h"
#include "debug.h"
#include "assert.h"
#include "Mutex.h"
#include "ScopeLock.h"
#include "File.h"

FileDescriptorList global_fd_list;

static size_t fd_num_ = 3;

FileDescriptor::FileDescriptor(File* file) :
    fd_(ArchThreads::atomic_add(fd_num_, 1)),
    file_(file), ref_count_(1)
{
  debug(VFS_FILE, "Create file descriptor %u\n", getFd());
}

FileDescriptor::~FileDescriptor()
{
  assert(this);
  debug(VFS_FILE, "Destroy file descriptor %p num %u\n", this, getFd());
}

void FileDescriptor::incrementRefCount()
{
  ref_count_.fetch_add(1);
  debug(FILEDESCRIPTOR, "Incremented reference count for global FD %d. Current count: %d\n",
        getFd(), getRefCount());
}

void FileDescriptor::decrementRefCount()
{
  ref_count_.fetch_add(-1);
  debug(FILEDESCRIPTOR, "Decremented reference count for global FD %d. Current count: %d\n",
        getFd(), getRefCount());
}
int FileDescriptor::getRefCount() const
{
  return ref_count_;
}


FileDescriptorList::FileDescriptorList() :
    fds_(), fd_lock_("File descriptor list lock")
{
}

FileDescriptorList::~FileDescriptorList()
{
  for(auto fd : fds_)
  {
    fd->getFile()->closeFd(fd);
  }

}

int FileDescriptorList::add(FileDescriptor* fd)
{
  debug(VFS_FILE, "FD list, add %p num %u\n", fd, fd->getFd());
  ScopeLock l(fd_lock_);

  for(auto x : fds_)
  {
    if(x->getFd() == fd->getFd())
    {
      return -1;
    }
  }

  fd->incrementRefCount();
  fds_.push_back(fd);

  return 0;
}

int FileDescriptorList::remove(FileDescriptor* fd)
{
  debug(VFS_FILE, "FD list, remove %p num %u\n", fd, fd->getFd());
  ScopeLock l(fd_lock_);
  for(auto it = fds_.begin(); it != fds_.end(); ++it)
  {
    if((*it)->getFd() == fd->getFd())
    {
      fds_.erase(it);

      fd->decrementRefCount();
      if (fd->getRefCount() == 0)
      {
        delete fd;
      }

      return 0;
    }
  }

  return -1;
}

FileDescriptor* FileDescriptorList::getFileDescriptor(uint32 fd_num)
{
  ScopeLock l(fd_lock_);
  for(auto fd : fds_)
  {
    if(fd->getFd() == fd_num)
    {
      assert(fd->getFile());
      return fd;
    }
  }

  return nullptr;
}
