#pragma once

#include "RingBuffer.h"
#include "Mutex.h"
#include "ScopeLock.h"
#include "Condition.h"
#include "sistream.h"
#include "debug.h"
#include "FileDescriptor.h"

class Pipe: public FileDescriptor {
public:

  Pipe(File *file, FileType type);
  ~Pipe();

  size_t read(char* buffer, size_t count);

  size_t write(const char* buffer, size_t size);

  size_t processBuffer(const char *buffer, size_t size);

  void close();

private:
  RingBuffer<char> buffer_;
  bool closed_;
  Mutex mtx;
  Condition cond_empty;
  Condition cond_full;
};