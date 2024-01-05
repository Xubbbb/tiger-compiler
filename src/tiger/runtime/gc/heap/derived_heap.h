#pragma once

#include "heap.h"
#include <vector>
#include "../roots/roots.h"

namespace gc {

struct string {
  int length;
  unsigned char chars[1];
};


/**
 * Notice: This is only a simulated heap
 * We will use copy collection algorithm(cheney) to implement our garbage collector
 * @param heap_start A pointer to a space on heap which is used as our simulated heap
 * @param heap_end A pointer to the end of the simulated heap
 * @param from A pointer to the start of the from-space
 * @param to A pointer to the start of the to-space
 * @param from_offset A offset to the start of the from-space
 * @param scan cheney algorithm's scan pointer
 * @param next cheney algorithm's next pointer
*/
class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
private:
  char* heap_start;
  char* heap_end;
  uint64_t from;
  uint64_t to;
  uint64_t from_offset;
  uint64_t scan;
  uint64_t next;

  Roots* roots;
public:
  DerivedHeap()
    : heap_start(nullptr), heap_end(nullptr), from(0), to(0), from_offset(0), scan(0), next(0), roots(nullptr)
  {
    roots = new Roots();
  }
  char *Allocate(uint64_t size) override;
  uint64_t Used() const override;
  uint64_t MaxFree() const override;
  void Initialize(uint64_t size) override;
  void GC() override;
  uint64_t* Forward(uint64_t* addr);
  struct string* newArrayLabel(int size);
};

} // namespace gc

