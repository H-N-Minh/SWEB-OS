#include "Pipe.h"
#include "FileSystemInfo.h"

#define O_RDONLY    0x0001
#define O_WRONLY    0x0002

Pipe::Pipe(File *file, FileType type)
    : FileDescriptor(file, type), buffer_(256), closed_(false), mtx("Pipe Mutex"),
      cond_empty(&mtx, "Pipe Empty Condition"), cond_full(&mtx, "Pipe Full Condition")
{
  debug(PIPE, "Pipe::Pipe called\n");
}

Pipe::~Pipe()
{
  debug(PIPE, "Pipe::~Pipe called\n");
}
size_t Pipe::write(const char* buffer, size_t size) {
  debug(PIPE, "Pipe::write called with buffer: %s\n", buffer);
  mtx.acquire();

  if (closed_)
  {
    debug(PIPE, "closed error\n");
    return -1;
  }

  size_t count = 0;
  while(count < size && !closed_)
  {
    if(!buffer_.isFull())
    {
      buffer_.put(buffer[count++]);
    }
    else
    {
      cond_full.wait();
    }

    if(count == size || buffer_.isFull())
    {
      cond_empty.signal();
    }
  }

  mtx.release();

  debug(PIPE, "Pipe::write: Wrote %zu bytes\n", count);
  return count;
}

size_t Pipe::read(char* buffer, size_t count) {
  debug(PIPE, "Pipe::read called\n");
  size_t num_read = 0;
  char c;

  mtx.acquire();

  while (num_read < count)
  {
    while (!buffer_.get(c) && !closed_) {
      cond_empty.wait();
    }
    buffer[num_read++] = c;
    cond_full.signal();
  }

  mtx.release();

  debug(PIPE, "Pipe::read: Read %zu bytes\n", num_read);
  return num_read;
}
void Pipe::close()
{
  debug(PIPE, "Closing pipe\n");
  closed_ = true;
  cond_full.signal();
  cond_empty.signal();
}
