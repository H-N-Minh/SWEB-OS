#pragma once

#include "RingBuffer.h"

class Pipe {
public:
  Pipe() : buffer_(256), closed_(false) {}

  bool read(char &c) {
    //could be impl: wait if buffer is empty
    if (buffer_.get(c)) {
      return true;
    }

    // case if pipe already closed:
    if (closed_) {
      //error or message or smthing
    }

    //more blockings or error handling
  }

  void write(char c) {
    //could be impl: wait if buffer is full
    if (!closed_) {
      buffer_.put(c);
    } else {
      //error for example error -1 as return
    }
  }

  void close() {
    closed_ = true;
  }

private:
  RingBuffer<char> buffer_;
  bool closed_;
  //synchronization and locking is ofc missing now
};