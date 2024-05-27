#include "stdlib.h"

void *malloc(size_t size)
{
  return (void*)__syscall(sc_malloc, size, 0x0, 0x0, 0x0, 0x0);
}

void free(void *ptr)
{
}

int atexit(void (*function)(void))
{
  return -1;
}

void *calloc(size_t nmemb, size_t size)
{
  return 0;
}

void *realloc(void *ptr, size_t size)
{
  return 0;
}
