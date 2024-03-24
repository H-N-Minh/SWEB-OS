#pragma once

#include "RingBuffer.h"
#include "Mutex.h"
#include "ScopeLock.h"
#include "Condition.h"
#include "sistream.h"

class Pipe {
public:

  Pipe() : buffer_(256), closed_(false), mtx("Pipe Mutex"), cond_empty(&mtx, "Pipe Empty Condition"), cond_full(&mtx, "Pipe Full Condition") {}

  bool read(char &c) {
    ScopeLock l(mtx);

    while (!closed_ && !buffer_.get(c)) {
      cond_empty.wait();
    }

    if (closed_ && !buffer_.get(c)) {
      c = EOF;
    } else {
      cond_full.signal();
    }

    cond_full.signal();
    return true;
  }

  bool write(char c) {
    ScopeLock l(mtx);

    while (!closed_ && buffer_.isFull()) {
      cond_full.wait();
    }

    if (closed_) {
      return false;
    }

    buffer_.put(c);
    cond_empty.signal();
    return true;
  }

  void close() {
    ScopeLock l(mtx);
    closed_ = true;
    cond_empty.signal();
    cond_full.signal();
  }

private:
  RingBuffer<char> buffer_;
  bool closed_;
  Mutex mtx;
  Condition cond_empty;
  Condition cond_full;
};