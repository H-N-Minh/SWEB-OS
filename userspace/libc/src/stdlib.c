#include "stdlib.h"
#include "string.h"


int malloc_counter = 0;
size_t used_block_counts_ = 0;
size_t free_bytes_left_on_page_ = 0;

MemoryBlock* first_memory_block_ = NULL;
MemoryBlock* heap_start__;

pthread_spinlock_t memory_lock;



void* malloc(size_t size)
{
  if(size == 0)
  {
    return NULL;
  }

  malloc_counter++;  //TODOs make this atomic
  if(malloc_counter == 1) 
  {
    heap_start__ = (MemoryBlock*)sbrk(0);
    pthread_spin_init(&memory_lock, 0);
  }

  pthread_spin_lock(&memory_lock);
  if(!first_memory_block_)
  {
    size_t bytes_needed = bytesNeededForMemoryBlock(size);
    int rv = allocateMemoryWithSbrk(bytes_needed);
    if(rv == -1)
    {
      pthread_spin_unlock(&memory_lock);
      return NULL;
    }

    first_memory_block_ = heap_start__;
    createNewMemoryBlock(first_memory_block_, size, 0, first_memory_block_ + 1, NULL);
    addOverflowProtection(first_memory_block_);
    used_block_counts_++;
    pthread_spin_unlock(&memory_lock);
    return first_memory_block_->address_;
  }
  else
  {
    MemoryBlock* next_memory_block = first_memory_block_;
    while(1)
    {
      if(checkOverflowProtection(next_memory_block) == -1)
      {
        pthread_spin_unlock(&memory_lock);
        exit(-1);
      }
      if(next_memory_block->is_free_ && next_memory_block->size_ >= size)
      {
        if(next_memory_block->size_ >= bytesNeededForMemoryBlock(size) + bytesNeededForMemoryBlock(0))
        {
          MemoryBlock* new_unused_memory_block = (MemoryBlock*)((size_t)next_memory_block + bytesNeededForMemoryBlock(size));
          createNewMemoryBlock(new_unused_memory_block, next_memory_block->size_ - bytesNeededForMemoryBlock(size), 1, new_unused_memory_block + 1, next_memory_block->next_);

          next_memory_block->next_ = new_unused_memory_block;
          next_memory_block->size_ = size;
          addOverflowProtection(next_memory_block);

          if(new_unused_memory_block->next_ && new_unused_memory_block->next_->is_free_)
          {
            new_unused_memory_block->size_ = new_unused_memory_block->size_ + bytesNeededForMemoryBlock(new_unused_memory_block->next_->size_);
            new_unused_memory_block->next_ = new_unused_memory_block->next_->next_;
          }
        }
        next_memory_block->is_free_ = 0;
        used_block_counts_++;
        pthread_spin_unlock(&memory_lock);
        return next_memory_block->address_;
      }
      //last element of linked list reached
      else if(next_memory_block->next_ == NULL)
      {
        if(bytesNeededForMemoryBlock(size) <= free_bytes_left_on_page_)
        {
          free_bytes_left_on_page_ -=  bytesNeededForMemoryBlock(size);
        }
        else
        {
          size_t bytes_needed = bytesNeededForMemoryBlock(size) - free_bytes_left_on_page_;
          int rv = allocateMemoryWithSbrk(bytes_needed);
          if(rv == -1)
          {
            pthread_spin_unlock(&memory_lock);
            return NULL;
          }
        }
        MemoryBlock* memory_block_new = (MemoryBlock*)((size_t)next_memory_block + bytesNeededForMemoryBlock(next_memory_block->size_));
        createNewMemoryBlock(memory_block_new, size, 0, memory_block_new + 1, NULL);
        addOverflowProtection(memory_block_new);
        used_block_counts_++;
        next_memory_block->next_ = memory_block_new;
        
        pthread_spin_unlock(&memory_lock);
        return memory_block_new->address_;
      }
      else
      {
        next_memory_block = next_memory_block->next_;
      }
    }
    pthread_spin_unlock(&memory_lock);
  }
}

void free(void *ptr)
{
  if(ptr == NULL) //TODOs (check pointer)
  {
    return;
  }

  pthread_spin_lock(&memory_lock);
  MemoryBlock* element_to_free = (MemoryBlock*)ptr - 1;
  MemoryBlock* next = first_memory_block_;
  if(next == NULL)
  {
     pthread_spin_unlock(&memory_lock);
    exit(-1);
  }
  MemoryBlock* element_before;
  while(next->next_ != NULL)
  {
    if(*((char*)((size_t)next->address_ + next->size_)) != '|')
    {
      pthread_spin_unlock(&memory_lock);
      exit(-1);
    }
    if(next->next_ == element_to_free)
    {
      element_before = next;
    }
    next = next->next_;
  }
  if(!element_before && element_to_free != first_memory_block_) //Check if element can be found in list
  {
    pthread_spin_unlock(&memory_lock);
    exit(-1);
  }
  if(element_to_free->is_free_) //check if element is already free
  {
    pthread_spin_unlock(&memory_lock);
    exit(-1);
  }
  used_block_counts_--;
  if(used_block_counts_ == 0)
  {
    free_bytes_left_on_page_ = 0;
    int rv = brk((void*)first_memory_block_);
    if(rv != 0)
    {
      pthread_spin_unlock(&memory_lock);
      exit(-1);
    }
    first_memory_block_ = NULL;
    pthread_spin_unlock(&memory_lock);
    return;
  }
  element_to_free->is_free_ = 1;

  if(element_to_free->next_ && element_to_free->next_->is_free_)
  {
    element_to_free->size_ = element_to_free->size_ + element_to_free->next_->size_ + sizeof(MemoryBlock) + sizeof(char);
    element_to_free->next_ = element_to_free->next_->next_;
  }
  if(element_to_free != first_memory_block_ &&element_before->is_free_)
  {
    element_before->size_ = element_before->size_ + element_before->next_->size_ + sizeof(MemoryBlock) + sizeof(char);
    element_before->next_ = element_before->next_->next_;
  }
  pthread_spin_unlock(&memory_lock);
}

int atexit(void (*function)(void))
{
  return -1;
}

void* calloc(size_t nmemb, size_t size)
{
  void* temp = malloc(nmemb * size);
  memset(temp, 0, nmemb * size);
  return temp; 

}

void *realloc(void *ptr, size_t size)
{
  return 0;
}


////Helper Functions:

size_t bytesNeededForMemoryBlock(size_t size)
{
  return size + sizeof(MemoryBlock) + sizeof(char);
}

int allocateMemoryWithSbrk(size_t bytes_needed)
{
  int buffer_size = 4096;
  size_t pages_needed = (bytes_needed / (buffer_size + 1)) + 1;
  free_bytes_left_on_page_ =  buffer_size - (bytes_needed % buffer_size);

  if(free_bytes_left_on_page_ == buffer_size)
  {
    free_bytes_left_on_page_ = 0;
  }

  void* rv = (void*)sbrk((ssize_t)(pages_needed * buffer_size));
  if(rv == (void*)-1)
  {
    return -1;
  }
  return 0;
}

void createNewMemoryBlock(MemoryBlock* memory_block, size_t size, int is_free, void* address, MemoryBlock* next)
{
  memory_block->size_ = size;
  memory_block->is_free_ = is_free;
  memory_block->address_ = address;
  memory_block->next_ = next;
}

void addOverflowProtection(MemoryBlock* memory_block)
{
  *((char*)((size_t)memory_block->address_ + memory_block->size_)) = '|';
}

int checkOverflowProtection(MemoryBlock* memory_block)
{
  if(*((char*)((size_t)memory_block->address_ + memory_block->size_)) == '|')
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

