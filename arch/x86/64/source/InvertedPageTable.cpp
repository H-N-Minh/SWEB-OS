#include "InvertedPageTable.h"
#include "assert.h"

void InvertedPageTable::addEntry(uint64_t physical_page, uint64_t virtual_page, uint64_t pid)
{
  assert(invertedPageTable.find(physical_page) == invertedPageTable.end() && "Entry already exists in Inverted Page Table");

  InvertedPageTableEntry newEntry;
  newEntry.virtual_page_ = virtual_page;
  newEntry.pid_ = pid;

  invertedPageTable[physical_page] = newEntry;
}

void InvertedPageTable::removeEntry(uint64_t physical_page)
{
  assert(invertedPageTable.find(physical_page) != invertedPageTable.end() && "Entry does not exist in Inverted Page Table");

  invertedPageTable.erase(physical_page);
}

InvertedPageTableEntry InvertedPageTable::getEntry(uint64_t physical_page)
{
  assert(invertedPageTable.find(physical_page) != invertedPageTable.end() && "Entry does not exist in Inverted Page Table");

  return invertedPageTable[physical_page];
}

