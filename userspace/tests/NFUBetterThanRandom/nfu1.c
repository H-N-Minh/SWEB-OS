#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096


#define PAGES_IN_ARRAY 2000
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8


size_t big_array[ELEMENTS_IN_ARRAY];

void printStatistic()
{
  int disk_writes;
  int disk_reads;
  int discard_unchanged_page;
  int reuse_same_disk_location;
  getSwappingStats(&disk_writes, &disk_reads, &discard_unchanged_page, &reuse_same_disk_location);
  printf("------------------------------------------------\n");
  printf("Total disk writes: %d\nTotal disk reads: %d\nDiscard unchange pages %d\nReuse same disk location %d\n", disk_writes, disk_reads, discard_unchanged_page, reuse_same_disk_location);
  printf("------------------------------------------------\n");
}


//Test for which nfu works better than random - aka going over 
int main()
{

   setPRA(__RANDOM_PRA__); 
    // setPRA(__NFU_PRA__); 


  calloc(800, 4096);
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }
  printf("hey1\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }
  for(int i = 0; i < 20; i++)
  {
    for(int j = 500; j < 520; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }
  printf("hey1\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }
  
  calloc(800, 4096);
printf("hey1\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }
printf("hey1\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }
printf("hey2\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }
printf("hey2\n");
  //   for(int i = 0; i < PAGES_IN_ARRAY; i++)
  // {
  //   big_array[i * (PAGESIZE / 8)] = (size_t)i * 13;    
  // }
 calloc(800, 4096);
printf("hey2\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }
printf("hey2\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }
printf("hey3\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }
  for(int i = 0; i < 20; i++)
  {
    for(int j = 500; j < 520; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

  




  printStatistic();

  return 0;
}













// #include "stdio.h"
// #include "pthread.h"
// #include "assert.h"
// #include "unistd.h"
// #include "sched.h"
// #include "nonstd.h"


// #define PAGESIZE 4096


// #define PAGES_IN_ARRAY 1500
// #define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8


// size_t big_array[ELEMENTS_IN_ARRAY];

// void printStatistic()
// {
//   int disk_writes;
//   int disk_reads;
//   int discard_unchanged_page;
//   int reuse_same_disk_location;
//   getSwappingStats(&disk_writes, &disk_reads, &discard_unchanged_page, &reuse_same_disk_location);
//   printf("------------------------------------------------\n");
//   printf("Total disk writes: %d\nTotal disk reads: %d\nDiscard unchange pages %d\nReuse same disk location %d\n", disk_writes, disk_reads, discard_unchanged_page, reuse_same_disk_location);
//   printf("------------------------------------------------\n");
// }


// //Test for which nfu works better than random - aka going over 
// int main()
// {

//    // setPRA(__RANDOM_PRA__); 
//     setPRA(__NFU_PRA__); 
//     // setPRA(__SECOND_CHANCE_PRA__); 

//   for(int i = 0; i < PAGES_IN_ARRAY; i++)
//   {
//     big_array[i * (PAGESIZE / 8)] = (size_t)i * 13;    
//   }

//   //working local 1
//   for(int i = 0; i < 20; i++)
//   {
//     for(int j = 20; j < 25; j++)
//     {
//       big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
//     }
//   }

//   printf("hey\n");

//  //working local 2
//   for(int i = 0; i < 20; i++)
//   {
//     for(int j = 40; j < 45; j++)
//     {
//       big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;    
//       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;        
//     }
//   }

//   printf("hey\n");

//   //working local 1
//   for(int j = 20; j < 25; j++)
//   {
//     assert(big_array[j * (PAGESIZE / 8)] == (size_t)j * 14);  
//   }

//   for(int i = 0; i < 20; i++)
//   {
//     for(int j = 20; j < 25; j++)
//     {
//        big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
//     }
//   }



//   for(int i = 0; i < PAGES_IN_ARRAY; i++)
//   {
//     assert(big_array[i * (PAGESIZE / 8)] == (size_t)i * 13);    
//   }



//   printStatistic();

//   return 0;
// }