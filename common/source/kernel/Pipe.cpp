#include "Pipe.h"


Pipe::Pipe()
    : buffer_(256),
      closed_(false),
      mtx("Pipe Mutex"),
      cond_empty(&mtx, "Pipe Empty Condition"),
      cond_full(&mtx, "Pipe Full Condition")
{
  debug(PIPE, "Pipe::Pipe called\n");
}

Pipe::~Pipe()
{
  debug(PIPE, "Pipe::~Pipe called\n");
}

bool Pipe::read(char &c) {
  debug(PIPE, "Pipe::read called\n");

  ScopeLock l(mtx);

  while (!closed_ && !buffer_.get(c)) {
    cond_empty.wait();
  }

  if (closed_ && !buffer_.get(c)) {
    c = EOF;
  } else {
    cond_full.signal();
  }

  //debug(PIPE, "Pipe::read read char: %c\n");

  cond_full.signal();
  return true;
}

bool Pipe::write(char c) {
  debug(PIPE, "Pipe::write called with char: %c\n", c);

  ScopeLock l(mtx);

  while (!closed_ && buffer_.isFull()) {
    cond_full.wait();
  }

  if (closed_) {
    return false;
  }

  buffer_.put(c);
  cond_empty.signal();

  debug(PIPE, "Pipe::write wrote char: %c\n", c);

  return true;
}

void Pipe::close() {
  debug(PIPE, "Pipe::close called\n");
  ScopeLock l(mtx);
  closed_ = true;
  cond_full.signal();
  cond_empty.signal();
}