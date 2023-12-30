#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

/**
 * Used for Tiger Module to generate pointer map
 * into .data section during compilation
*/
class AssemPointerMap{
public:
  std::string label_;
  std::string next_label_;
  std::string key_label_;
  int framesize;
  std::vector<int> root_offsets;
  std::string endmap;
};

/**
 * Used for GC Module to get roots
*/
class GCPointerMap{
public:
  char* next_addr_;
  char* key_addr_;
  int framesize;
  std::vector<int> root_offsets;
};


class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
};

}

#endif // TIGER_RUNTIME_GC_ROOTS_H