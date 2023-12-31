#include "derived_heap.h"
#include <stdio.h>
#include <stack>
#include <cstring>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
char* DerivedHeap::Allocate(uint64_t size) {
  auto res = from + from_offset;
  /**
   * 1. Check if there is enough space in from-space
  */
  if((heap_start == from && res + size > to) || (res + size > heap_end)){
    return nullptr;
  }
  from_offset += size;
  return res;
}

uint64_t DerivedHeap::Used() const {
  return from_offset;
}

uint64_t DerivedHeap::MaxFree() const {
  return (heap_end - heap_start) / 2 - from_offset;
}

void DerivedHeap::Initialize(uint64_t size) {
  heap_start = new char[size];
  heap_end = heap_start + size;
  from = heap_start;
  to = heap_start + size / 2;
  from_offset = 0;
  scan = nullptr;
  next = nullptr;
  ArrayLabel = new char[6];
  strcpy(ArrayLabel, "Array");
  RecordLabel = new char[7];
  strcpy(RecordLabel, "Record");
}

void DerivedHeap::GC() {
  //TODO
}

} // namespace gc

