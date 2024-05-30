#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "sched.h"


#define MEGABYTE 1048576
#define PAGESIZE 4096

#define N 200
#define N_MEGABYTE N * MEGABYTE

#define ELEMENTS_IN_ARRAY N_MEGABYTE / 8
#define PAGES_IN_ARRAY N_MEGABYTE/PAGESIZE

size_t big_array2[ELEMENTS_IN_ARRAY];
static unsigned int randomSeed = 0;

void srand(unsigned int seed) {
  randomSeed = seed;
}

unsigned int rand(void) {
  randomSeed = randomSeed * 214013 + 2531011;
  return (randomSeed >> 16) & 0x7FFF;
}

int ipt4()
{
  printf("Starting ipt4 test...\n");
  size_t count_pages = N_MEGABYTE/PAGESIZE;

  // random seed
  srand(42);

  // iterate over all page frames once
  printf("Randomly modifying %zu pages\n", count_pages);
  for(size_t i = 0; i < count_pages; i++)
  {
    // generate random page number
//    printf("Modifying page %zu\n", i);
    size_t page = rand() % count_pages;

    // Modify a byte on the page
    big_array2[page * (PAGESIZE / 8)] = (size_t)i;
  }
  printf("Done\n");

  // check the correctness of the data via extra read loop
  printf("Checking correctness\n");
  for(size_t i = 0; i < count_pages; i++)
  {
//    printf("Checking page %zu\n", i);
    size_t page = i % count_pages;
    assert(big_array2[page * (PAGESIZE / 8)] <= (size_t)count_pages);
  }
  printf("Done2\n");
  return 0;
}

