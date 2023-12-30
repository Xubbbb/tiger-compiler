#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */
public:
  const int K = 15;
  assem::InstrList* instr_list;
  fg::FlowGraphFactory* flow_fac;
  live::LiveGraphFactory* live_fac;
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

  Color(assem::InstrList* il):instr_list(il){}
  ~Color(){}
  col::Result RAColor();
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
};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
