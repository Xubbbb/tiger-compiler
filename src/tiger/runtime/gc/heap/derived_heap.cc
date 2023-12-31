#include "derived_heap.h"
#include "tiger/runtime/gc/roots/roots.h"
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
  auto roots = new Roots();
  roots->findRoots();
}

void Roots::findRoots(){
  uint64_t* sp;
  GET_TIGER_STACK(sp);
  GCPointerMap pointer_map;
  uint64_t* frame_it = sp;
  while (true){
    bool flag = false;
    for(int i = 0;i < pointermaps_.size();++i){
      if((uint64_t)pointermaps_[i].key_addr_ == *frame_it){
        pointer_map = pointermaps_[i];
        flag = true;
        break;
      }
    }
    if(!flag){
      break;
    }
    // Don't forget to skip the return address
    frame_it += pointer_map.framesize / TigerHeap::WORD_SIZE + 1;
    for(int i = 0;i < pointer_map.root_offsets.size();++i){
      auto root_ptr = (uint64_t*)(*(frame_it + (pointer_map.root_offsets[i] / TigerHeap::WORD_SIZE - 1)));
      //!debug//
      std::cout << "root_ptr : " << root_ptr << std::endl;
      //!debug//
      roots_.push_back(root_ptr);
    }
  }
}

void Roots::getPointerMaps(){
  uint64_t *pointer_map_begin = &GLOBAL_GC_ROOTS;
  uint64_t *reader_it = pointer_map_begin;
  while(true){
    GCPointerMap pointer_map;
    pointer_map.next_addr_ = (char*)(*reader_it);
    reader_it++;
    pointer_map.key_addr_ = (char*)(*reader_it);
    reader_it++;
    pointer_map.framesize = (int)(*reader_it);
    reader_it++;
    while(true){
      int offset = (int)(*reader_it);
      if(offset == 1){
        break;
      }
      pointer_map.root_offsets.push_back(offset);
      reader_it++;
    }
    pointermaps_.push_back(pointer_map);
    if(pointer_map.next_addr_ == nullptr){
      break;
    }
  }
  //!debug//
  std::cout << "getPointerMaps finished" << std::endl;
  for(int i = 0;i < pointermaps_.size();++i){
    std::cout << "pointer_map label" << i << " : " << std::endl;
    std::cout << "next_addr_ : " << pointermaps_[i].next_addr_ << std::endl;
    std::cout << "key_addr_ : " << pointermaps_[i].key_addr_ << std::endl;
    std::cout << "framesize : " << pointermaps_[i].framesize << std::endl;
    std::cout << "root_offsets : " << std::endl;
    for(int j = 0;j < pointermaps_[i].root_offsets.size();++j){
      std::cout << pointermaps_[i].root_offsets[j] << " ";
    }
    std::cout << std::endl;
  }
  //!debug//
}

} // namespace gc

