#include "stdlib.h"

void *malloc(size_t size)
{
  return (void*)__syscall(sc_malloc, size, 0x0, 0x0, 0x0, 0x0);
}

void free(void *ptr)
{
  __syscall(sc_free, (size_t)ptr, 0x0, 0x0, 0x0, 0x0);
}

int atexit(void (*function)(void))
{
  return -1;
}

void *calloc(size_t nmemb, size_t size)
{
  return (void*)__syscall(sc_calloc, nmemb, size, 0x0, 0x0, 0x0);;
}

void *realloc(void *ptr, size_t size)
{
  return 0;
}
