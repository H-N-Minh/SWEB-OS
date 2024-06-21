#include "assert.h"
#include "stdlib.h"

//Big malloc
int malloc2()
{
  
  int* ptr = malloc(4096000);
  assert(ptr != 0);
  free(ptr);
  return 0;
}