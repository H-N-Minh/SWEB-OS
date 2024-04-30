#include "Pipe.h"


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
size_t Pipe::read(char* buffer, size_t count) {
  debug(PIPE, "Pipe::read called\n");

  ScopeLock l(mtx);

  size_t num_read = 0;
  char c;

  while (num_read < count && !closed_){
    while (!closed_ && !buffer_.get(c)) {
      cond_empty.wait();
    }

    if (closed_ && !buffer_.get(c)) {
      return num_read;
    }

    buffer[num_read++] = c;

    cond_full.signal();
  }

  debug(PIPE, "Pipe::read: Read %zu bytes\n", num_read);
  return num_read;
}
size_t Pipe::write(const char* buffer, size_t size) {
  debug(PIPE, "Pipe::write called with buffer: %s\n", buffer);

  ScopeLock l(mtx);

  size_t count = 0;

  while (count < size && !closed_ && buffer_.isFull()) {
    cond_full.wait();
  }

  while (count < size && !closed_) {
    if (buffer_.isFull()) {
      cond_full.wait();
    } else {
      buffer_.put(buffer[count++]);
    }
  }

  if (closed_) {
    return -1;
  }

  cond_empty.signal();

  debug(PIPE, "Pipe::write: Wrote %zu bytes\n", count);

  return count;
}

void Pipe::close() {
  debug(PIPE, "Pipe::close called\n");
  ScopeLock l(mtx);
  closed_ = true;
  cond_full.signal();
  cond_empty.signal();
}