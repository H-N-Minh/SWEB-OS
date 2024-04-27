#pragma once

#include "RingBuffer.h"
#include "Mutex.h"
#include "ScopeLock.h"
#include "Condition.h"
#include "sistream.h"
#include "debug.h"

class Pipe {
public:

  Pipe();
  ~Pipe();

  bool read(char &c);

  bool write(char c);

  void close();

private:
  RingBuffer<char> buffer_;
  bool closed_;
  Mutex mtx;
  Condition cond_empty;
  Condition cond_full;
};