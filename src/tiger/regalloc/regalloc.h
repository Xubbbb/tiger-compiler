#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result(){}
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  frame::Frame* frame_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  col::Color* color_;
  temp::Map* coloring_;

  RegAllocator(frame::Frame* frame, std::unique_ptr<cg::AssemInstr> assem_instr);
  ~RegAllocator();
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();
  void RewriteProgram(live::INodeListPtr spilledNodes);
};

} // namespace ra

#endif