#pragma once

#include "types.h"
#include "Mutex.h"
#include "ArchMemory.h"
#include "ElfFormat.h"
#include <uvector.h>
#include <ulist.h>

class Stabs2DebugInfo;

class Loader
{
  public:
    Loader(ssize_t fd);
    Loader(const Loader &src, int32 fd, ustl::vector<size_t>& preallocated_pages);
    ~Loader();

    /**
     * Loads the ehdr and phdrs from the executable and (optionally) loads debug info.
     * @return true if this was successful, false otherwise
     */
    bool loadExecutableAndInitProcess();

    /**
     * Loads one binary page by its virtual address:
     * Gets a free physical page, copies the contents of the binary to it, and then maps it.
     * @param virtual_address virtual address where to find the page to load
     * @param preallocated_pages A vector of 4 preallocated ppn
     */
    void loadPage(pointer virtual_address, ustl::vector<size_t>& preallocated_pages);

    /**
     * Return the start address of Heap
    */
    void* getBrkStart();

    Stabs2DebugInfo const* getDebugInfos() const;

    void* getEntryFunction() const;

    void replaceLoader(int32 execv_fd);

    ArchMemory arch_memory_;

  private:

    /**
     *reads ELF-headers from the executable
     * @return true if this was successful, false otherwise
     */
    bool readHeaders();


    /**
     * clean up and sort the elf headers for faster access.
     * @return true in case the headers could be prepared
     */
    bool prepareHeaders();


    bool loadDebugInfoIfAvailable();


    bool readFromBinary (char* buffer, l_off_t position, size_t length);


    size_t fd_;
    Elf::Ehdr *hdr_;
    ustl::list<Elf::Phdr> phdrs_;
    Mutex program_binary_lock_;

    Stabs2DebugInfo *userspace_debug_info_;

};

