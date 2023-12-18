#include "tiger/liveness/flowgraph.h"

namespace fg {

// generate a flow graph from instruction list
void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  auto instr_list = instr_list_->GetList();
  /**
   * pred is a pointer to the previous instruction node, usually we should link pred and current node
   * except pred is a jump related instruction
  */
  FNodePtr pred = nullptr;
  std::vector<FNodePtr> jump_related_nodes;
  for(auto instr_it = instr_list.begin();instr_it != instr_list.end();++instr_it){
    auto new_node = flowgraph_->NewNode(*instr_it);
    if(typeid(*(*instr_it)) == typeid(assem::LabelInstr)){
      auto label_instr = static_cast<assem::LabelInstr *>(*instr_it);
      label_map_->Enter(label_instr->label_, new_node);
    }
    if(pred != nullptr){
      if(typeid(*(pred->NodeInfo())) == typeid(assem::OperInstr)){
        auto oper_instr = static_cast<assem::OperInstr *>(pred->NodeInfo());
        // This means this is a jump related instr
        if(oper_instr->jumps_ != nullptr){
          // do nothing, wait until all label is pushed into map then go
          // back to fill it
          jump_related_nodes.push_back(pred);
        }
        else{
          flowgraph_->AddEdge(pred, new_node);
        }
      }
      else{
        flowgraph_->AddEdge(pred, new_node);
      }
    }
    pred = new_node;
  }
  for(auto node_it = jump_related_nodes.begin();node_it != jump_related_nodes.end();++node_it){
    auto oper_instr = static_cast<assem::OperInstr *>((*node_it)->NodeInfo());
    auto jump_list = oper_instr->jumps_->labels_;
    for(auto label_it = jump_list->begin();label_it != jump_list->end();++label_it){
      auto jump_target_node = label_map_->Look(*label_it);
      if(jump_target_node == nullptr){
        // error
        return;
      }
      flowgraph_->AddEdge(*node_it, jump_target_node);
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
