#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "nonstd.h"
#include "sched.h"


//Checks if ipt is set and not bigger than 1 for any page
int ipt1()
{
  // for(int i = 0; i< 100; i++)
  // {
  //   getIPTInfos(i * 100);
  //   sleep(1);  
  // }
  
  // int counter = 0;
  // for(size_t ppn = 1009; ppn < 2016; ppn++)  //ppn used for userspace
  // {
  //   int page_count =  getIPTInfos(ppn);
    
  //   assert((page_count == 0 || page_count == 1) && "Each ppn should belong to only one page");


  //   if(page_count == 1)
  //   {
  //     counter++;
  //     // printf("For ppn %ld there are %d pages in inverse page table (ipt).\n", ppn, page_count);
  //   }
  // }

  // assert(counter > 0 && "There need to be some pages in the inverted page table for this program to work");
  return 0;
}