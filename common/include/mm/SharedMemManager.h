#pragma once
#include "ArchMemory.h"
#include "FileDescriptor.h"
#include "Mutex.h"
#include "types.h"
#include "umultimap.h"
#include "Mutex.h"
#include "ustring.h"
#include "FileDescriptor.h"
//#include "FileDescriptorList.h"
#include "debug.h"
#include "ArchMemory.h"


#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010
#define PROT_EXEC     0x00000004  // ..0100

#define MAP_PRIVATE   0x20000000  // 0010..
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..

#define MAP_FAILED	((void *) -1)

typedef size_t vpn_t;
extern FileDescriptorList global_fd_list;

class SharedMemObject;

// struct to store parameters for mmap
typedef struct mmap_params {
	void* start;
	size_t length;
	int prot;
	uint32 flags;
	int fd;
	ssize_t offset;
}mmap_params_t;




// struct to store all information of each shared memory entry within a process
class SharedMemEntry
{
public:
	vpn_t start_;
	vpn_t end_;
	int prot_;
	int flags_;
	int fd_;
	ssize_t offset_;
	bool shared_;
	FileDescriptor* globalFileDescriptor_;

	SharedMemEntry(vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset, bool shared, FileDescriptor* globalFileDescriptor);
	SharedMemEntry(const SharedMemEntry& other);
	~SharedMemEntry();

	/**
   * check if the given vpn is within this shared memory block
	 */
	bool isInBlockRange(vpn_t vpn);

	/**
   * get the size of the mem block (in pages)
	 */
	size_t getSize();

	/**
   * get the offset of the given vpn from the start of the shared memory block.
   * @return in bytes. Assert if vpn is not within the block
	 */
	ssize_t getOffset(size_t vpn);
};


class SharedMemManager
{
private:
	ustl::vector<SharedMemEntry*> shared_map_;
	vpn_t last_free_vpn_;

	static ustl::map<ustl::string, SharedMemObject*>* shm_objects_;

public:
	Mutex shared_mem_lock_; // responsible for shared_map_ and last_free_vpn_
	// locking order: shared_mem_lock_ -> ipt_lock_ -> archmem_lock_

	SharedMemManager();
	SharedMemManager(const SharedMemManager& other);
	~SharedMemManager();


	void* mmap(mmap_params_t* params);
	int munmap(void* addr, size_t length);

	/**
   * add an entry to the shared memory map
   * @return the starting address of the shared memory region
   * @return MAP_FAILED if the shared memory region is full
	 */
	void* addEntry(void* addr, size_t length, int prot, int flags, int fd, ssize_t offset, bool shared);

	/**
   * check if the given address is within any shared memory block
	 */
	bool isAddressValid(vpn_t vpn);

	/**
   * handle the page fault that is related to shared memory. also Setting the protections for the page
	 */
	void handleSharedPF(ustl::vector<uint32>& preallocated_pages, size_t address);

	/**
   * get the shared memory entry that contains the given address
	 */
	SharedMemEntry* getSharedMemEntry(size_t address);

	/**
   * unmap one page from the shared memory. The vpn should already be a valid page that can be remove
	 */
	void unmapOnePage(vpn_t vpn, SharedMemEntry *sm_entry,
										FileDescriptor *pDescriptor);

	/**
   * find all pages in shared mem that are being used and are within the given range and fill up the vector relevant_pages with them
   * This vector is then used to unmap all pages in the range
   * @param relevant_pages: an empty vector to be filled up
   * @param start: the starting address of the range
   * @param length: the length of the range (in bytes)
   * @return : the vector filled up with all relevant pages. Vector is empty if error (if theres at least 1 vpn in the range that is not an active shared mem page)
	 */
	void findRelevantPages(ustl::vector<ustl::pair<vpn_t, SharedMemEntry*>> &relevant_pages, size_t start, size_t length);

	/**
   * unmap all active shared mem pages. Used for process termination
	 */
	void unmapAllPages(ArchMemory* arch_memory);

	/**
   * helper for HandleSharedPF(), set the protection bits for the new vpn, based on the info from the shared memory entry
	 */
	void setProtectionBits(SharedMemEntry* entry, ArchMemory* archmem, size_t vpn);

	/**
   * read the fd file and copy content (1 page, start from the offset) to the given ppn
	 */
	void copyContentFromFD(size_t ppn, int fd, ssize_t offset, ArchMemory* arch_memory, FileDescriptor *globalFileDescriptor);

	/**
   * write the content of the given vpn to the fd file (1 page, start from the offset)
	 */
	void writeBackToFile(size_t vpn, int fd, ssize_t offset, ArchMemory* arch_memory, FileDescriptor *global_fd_obj);

	/**
   * check if the given page is a shared page and only 1 process left using it. If so, write back to the file before unmap
	 */
	bool isTimeToWriteBack(SharedMemEntry* sm_entry, ArchMemory* arch_memory, size_t vpn);


	int shm_open(char* name, size_t oflag, mode_t mode);
	int shm_unlink(char* name);
	int ftruncate(int fd, off_t length);

};

// struct to store parameters for shared memory object
class SharedMemObject {
public:
	SharedMemObject() {}

	static ustl::string* name_;
	static FileDescriptor* global_fd_;

	static SharedMemObject* Init(const ustl::string& name)
	{
		if (!name_)
			name_ = new ustl::string(name); // TODO: free!!!
		else
			*name_ = name;

		global_fd_ = new FileDescriptor(nullptr, FileDescriptor::FileType::SHARED_MEMORY); // TODO: free!!!
		return new SharedMemObject(); // TODO: free!!!
	}

	static void Cleanup()
	{
		delete name_;
		delete global_fd_;
		name_ = nullptr;
		global_fd_ = nullptr;
	}

	static FileDescriptor* getGlobalFileDescriptor();
};
