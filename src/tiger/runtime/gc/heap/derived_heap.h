#pragma once

#include "heap.h"
#include <vector>
#include "../roots/roots.h"

namespace gc {
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
  char* from;
  char* to;
  uint64_t from_offset;
  char* scan;
  char* next;

  char* ArrayLabel;
  char* RecordLabel;
  Roots* roots;
public:
  DerivedHeap()
    : heap_start(nullptr), heap_end(nullptr), from(nullptr), to(nullptr), from_offset(0), scan(nullptr), next(nullptr), ArrayLabel(nullptr), RecordLabel(nullptr), roots(nullptr)
  {
    roots = new Roots();
  }
  char *Allocate(uint64_t size) override;
  uint64_t Used() const override;
  uint64_t MaxFree() const override;
  void Initialize(uint64_t size) override;
  void GC() override;
  char* getArrayLabel() const{
    return ArrayLabel;
  }
  char* getRecordLabel() const{
    return RecordLabel;
  }
};

} // namespace gc

