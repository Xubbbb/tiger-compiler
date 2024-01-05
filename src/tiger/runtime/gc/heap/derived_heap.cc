#include "derived_heap.h"
#include "../roots/roots.h"
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
  if(((uint64_t)heap_start == from && res + size > to) || (res + size > (uint64_t)heap_end)){
    return nullptr;
  }
  from_offset += size;
  return (char*)res;
}

uint64_t DerivedHeap::Used() const {
  return from_offset;
}

uint64_t DerivedHeap::MaxFree() const {
  return (heap_end - heap_start) / 2 - from_offset;
}

void DerivedHeap::Initialize(uint64_t size) {
  /**
   * Because we use copying collection algorithm, 
   * we need 2x heap size(other wise our heap will be still full after one GC)
  */
  size = size << 1;
  heap_start = new char[size];
  heap_end = heap_start + size;
  from = (uint64_t)heap_start;
  to = (uint64_t)heap_start + size / 2;
  from_offset = 0;
  scan = 0;
  next = 0;
}

void DerivedHeap::GC() {
  /**
   * ?
   * I can't understand why I can access correct return address
   * only in GC() to call GET_TIGER_STACK(sp). I think I should
   * call GET_TIGER_STACK(sp) in alloc_record at first, but it
   * doesn't work.
  */
  uint64_t* sp;
  GET_TIGER_STACK(sp);
  roots->findRoots(sp);
  // printf("from_offset before GC: %ld\n", from_offset);
  scan = to;
  next = to;
  for(auto root : roots->roots_){
    *root = (uint64_t)Forward(root);
  }
  while(scan < next){
    auto descriptor_ptr = (struct string *)(*(uint64_t*)scan);
    auto chars_length = descriptor_ptr->length;
    //! When we use 'auto descriptor = *descriptor_ptr;'
    //! the content in chars will be changed, I don't know why???
    // auto descriptor = *descriptor_ptr;
    std::string str = "";
    for(int i = 0;i < chars_length;++i){
      str += (char)(descriptor_ptr->chars[i]);
    }
    if(str.find("Array") != std::string::npos){
      auto array_length = std::stoi(str.substr(5, str.length() - 5));
      scan += (array_length + 1) * TigerHeap::WORD_SIZE;
    }
    else{
      for(int i = 0;i < chars_length;++i){
        if(descriptor_ptr->chars[i] == '1'){
          auto field_ptr = (uint64_t*)(scan) + 1 + i;
          *field_ptr = (uint64_t)Forward(field_ptr);
        }
      }
      scan += (chars_length + 1) * TigerHeap::WORD_SIZE;
    }
  }

  /**
   * Swap from-space and to-space
  */
  from_offset = next - to;
  auto tmp = from;
  from = to;
  to = tmp;
  roots->roots_.clear();
  // printf("from_offset after GC: %ld\n", from_offset);
}

uint64_t* DerivedHeap::Forward(uint64_t* addr){
  if(*addr >= WORD_SIZE){
    auto obj_head = *addr - WORD_SIZE;
    if(obj_head >= from && obj_head < from + from_offset){
      auto descriptor_ptr = (struct string *)(*((uint64_t*)obj_head));
      //! When we use 'auto descriptor = *descriptor_ptr;'
      //! the content in chars will be changed, I don't know why???
      // auto descriptor = *descriptor_ptr;
      auto chars_length = descriptor_ptr->length;
      std::string str = "";
      for(int i = 0;i < chars_length;++i){
        str += (char)(descriptor_ptr->chars[i]);
      }
      if(str.find("Array") != std::string::npos){
        auto array_length = std::stoi(str.substr(5, str.length() - 5));
        auto new_obj = next;
        next += (array_length + 1) * TigerHeap::WORD_SIZE;
        memcpy((void*)new_obj, (void*)obj_head, (array_length + 1) * TigerHeap::WORD_SIZE);
        return (uint64_t*)(new_obj + WORD_SIZE);
      }
      else{
        auto p_f1 = (uint64_t*)(obj_head + WORD_SIZE);
        if(*p_f1 >= WORD_SIZE && ((*p_f1 - WORD_SIZE) >= to && (*p_f1 - WORD_SIZE) < to + ((uint64_t)(heap_end - heap_start)) / 2)){
          return (uint64_t*)(*p_f1);
        }
        else{
          auto new_obj = next;
          next += (chars_length + 1) * TigerHeap::WORD_SIZE;
          memcpy((void*)new_obj, (void*)obj_head, (chars_length + 1) * TigerHeap::WORD_SIZE);
          *p_f1 = new_obj + WORD_SIZE;
          return (uint64_t*)(*p_f1);
        }
      }
    }
    else{
      return (uint64_t*)(*addr);
    }
  }
}

void Roots::findRoots(uint64_t* sp){
  GCPointerMap pointer_map;
  uint64_t* frame_it = sp;
  while (true){
    bool flag = false;
    /**
     * ?
     * Why I will encounter a segmentation fault when I use std::cout
     * But printf works well
    */
    // printf("frame_it : %p\n", (uint64_t*)(*frame_it));s
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
      // auto root_ptr = (uint64_t*)(*(frame_it + (pointer_map.root_offsets[i] / TigerHeap::WORD_SIZE)));
      auto root_ptr = frame_it + (pointer_map.root_offsets[i] / TigerHeap::WORD_SIZE);
      //!debug//
      // printf("root_ptr : %p\n", root_ptr);
      // printf("root_ptr value : %p\n", (uint64_t*)(*root_ptr));
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
      reader_it++;
      if(offset == 1){
        break;
      }
      pointer_map.root_offsets.push_back(offset);
    }
    pointermaps_.push_back(pointer_map);
    if(pointer_map.next_addr_ == nullptr){
      break;
    }
  }
  //!debug//
  // printf("getPointerMaps finished\n");
  // for(int i = 0;i < pointermaps_.size();++i){
  //   printf("pointer_map label %d : \n", i);
  //   printf("next_addr_ : %p\n", (uint64_t*)pointermaps_[i].next_addr_);
  //   printf("key_addr_ : %p\n", (uint64_t*)pointermaps_[i].key_addr_);
  //   printf("framesize : %d\n", pointermaps_[i].framesize);
  //   printf("root_offsets : \n");
  //   for(int j = 0;j < pointermaps_[i].root_offsets.size();++j){
  //     printf("%d ", pointermaps_[i].root_offsets[j]);
  //   }
  //   printf("\n");
  // }
  //!debug//
}

struct string *DerivedHeap::newArrayLabel(int size){
  std::string str = "Array";
  str += std::to_string(size);
  size_t struct_size = sizeof(struct string) + str.size();
  struct string* res = (struct string*)malloc(struct_size);
  res->length = str.size();
  memcpy(res->chars, str.data(), str.size());
  std::string str2((char*)res->chars, res->length);
  return res;
}

} // namespace gc

