#include "assert.h"
#include "stdlib.h"

int realloc1()
{	
	//realloc with no ptr should work like a normal malloc
  void* break_before = sbrk(0);

	int* ptr1 = realloc(NULL, 20 * sizeof(int));
	printf("ptr1: %p\n", ptr1);
	assert(ptr1 != NULL);
	for(int i = 0; i < 20; i++)
	{
		ptr1[i] = i;
	}
	
	//saved stuff should still be in ptr after increasing the size
	int* ptr2 = realloc(ptr1, 40 * sizeof(int));
	printf("ptr2: %p\n", ptr2);
	assert(ptr2 != NULL);
	assert(ptr1 == ptr2 && "Since there is nothing behind us we can just reuse the same position");
	for(int i = 0; i < 20; i++)
	{
		assert(ptr2[i] == i);
	}
	for(int i = 20; i < 40; i++)
	{
		ptr2[i] = i;
	}
	
	//Force memoryblock to move location, should still has its saved stuff inside
	int* ptr3 = malloc(20 * sizeof(int));
	assert(ptr3 != NULL);
	int* ptr4 = realloc(ptr2, 40 * sizeof(int));
	assert(ptr4 != NULL);
	printf("ptr3: %p\n", ptr3);
	printf("ptr4: %p\n", ptr4);
	assert(ptr4 > ptr3 && "Make sure it moved the location");
	assert(ptr3 > ptr2 && "Make sure it moved the location");

	
	for(int i = 0; i < 40; i++)
	{
		assert(ptr4[i] == i);
	}
	
	//Make sure that old location is usable again
	int* ptr5 = malloc(20 * sizeof(int));
	assert(ptr5 != NULL);
	assert(ptr5 < ptr4);
	
	free(ptr3);
	free(ptr5);
	free(ptr4);

  void* break_after = sbrk(0);
  assert(break_before - break_after == 0);
	
	return 0;
}
