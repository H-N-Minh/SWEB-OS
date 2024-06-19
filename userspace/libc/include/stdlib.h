// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Andreas Niederl
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#pragma once

#include "types.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "pthread.h"
#include "unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * exit value constants
 *
 */
#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

/**
 * NULL pointer constant
 *
 */
#define NULL 0

struct MemoryBlock
{
  int is_free_;
  size_t size_;
  void* address_;
  struct MemoryBlock* next_;
};
typedef struct MemoryBlock MemoryBlock;


/**
 * parses the given string and returns its value as integer
 * @param string the string for parsing
 * @return the parsed value
 *
 */
extern int atoi(const char *string);

/**
 * parses the given string and returns its value as long
 * @param string the string for parsing
 * @return the parsed value
 *
 */
extern long atol(const char *string);

/**
 * Terminates the program normally, functions registered by atexit() are called
 * in reverse order of their registration, any open file descriptors belonging
 * to the process are closed, any children of the process are inherited by
 * process 1 (init) and the process's parent is sent a SIGCHLD signal.
 * The status value is returned to the parent as exit status and can be
 * collected using a wait call.
 * @param status the exit status of the calling process, 0 or EXIT_SUCCESS for
 * successfull termination, EXIT_FAILURE for unsuccessful termination
 *
 */
extern void exit(int status);

/**
 * register a function to be called at normal process termination
 * @param function the function to be called
 * @return 0 if successful, other-wise non-zero
 */
extern int atexit(void (*function)(void));

extern void *malloc(size_t size);

extern void *calloc(size_t nmemb, size_t size);

extern void *realloc(void *ptr, size_t size);

extern void free(void *ptr);

extern size_t bytesNeededForMemoryBlock(size_t size);
extern int allocateMemoryWithSbrk(size_t bytes_needed);
extern void createNewMemoryBlock(MemoryBlock* memory_block, size_t size, int is_free, void* address, MemoryBlock* next);
extern void addOverflowProtection(MemoryBlock* memory_block);
extern int checkOverflowProtection(MemoryBlock* memory_block);

#ifdef __cplusplus
}
#endif



