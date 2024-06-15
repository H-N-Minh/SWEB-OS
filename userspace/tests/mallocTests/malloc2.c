#include "assert.h"
#include "stdlib.h"


int malloc2()
{
  
  int* ptr = malloc(4096000);

  free(ptr);
  return 0;
}