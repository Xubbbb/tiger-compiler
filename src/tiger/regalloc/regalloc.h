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
private:
  const int K = 15;
  frame::Frame* frame_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  live::IGraphPtr ig;

  live::INodeListPtr precolored;

  live::INodeListPtr simplifyWorklist;
  live::INodeListPtr freezeWorklist;
  live::INodeListPtr spillWorklist;
  live::INodeListPtr spilledNodes;
  live::INodeListPtr coalescedNodes;
  live::INodeListPtr coloredNodes;
  live::INodeListPtr selectStack;

  live::MoveList* coalescedMoves;
  live::MoveList* constrainedMoves;
  live::MoveList* frozenMoves;
  live::MoveList* worklistMoves;
  live::MoveList* activeMoves;

  graph::Table<temp::Temp, int>* degree;
  graph::Table<temp::Temp, live::MoveList>* moveList;
  graph::Table<temp::Temp, live::INode>* alias;
  graph::Table<temp::Temp, temp::Temp>* color;
  

public:
  RegAllocator(frame::Frame* frame, std::unique_ptr<cg::AssemInstr> assem_instr);
  ~RegAllocator();
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();
  void Build();
  void AddEdge(live::INodePtr u, live::INodePtr v);
  void MakeWorklist();
  live::INodeListPtr Adjacent(live::INodePtr n);
  live::MoveList* NodeMoves(live::INodePtr n);
  bool MoveRelated(live::INodePtr n);
  void Simplify();
  void DecrementDegree(live::INodePtr m);
  void EnableMoves(live::INodeListPtr nodes);
  void Coalesce();
  void AddWorkList(live::INodePtr u);
  bool OK(live::INodePtr t, live::INodePtr r);
  bool Conservative(live::INodeListPtr nodes);
  live::INodePtr GetAlias(live::INodePtr n);
  void Combine(live::INodePtr u, live::INodePtr v);
  void Freeze();
  void FreezeMoves(live::INodePtr u);
  void SelectSpill();
  void AssignColors();
  void RewriteProgram();
  void DebugOut();
};

} // namespace ra

#endif