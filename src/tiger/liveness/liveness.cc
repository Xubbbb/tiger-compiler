#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

temp::TempList* LiveGraphFactory::CalculateNewIn(temp::TempList* use, temp::TempList* old_out, temp::TempList* def){
  auto res = new temp::TempList();
  temp::TempList old_def;
  if(old_out != nullptr){
    auto old_out_list = old_out->GetList();
    if(def != nullptr){
      for(auto old_it = old_out_list.begin();old_it != old_out_list.end();++old_it){
        bool is_in_def = false;
        auto def_list = def->GetList();
        for(auto def_it = def_list.begin();def_it != def_list.end();++def_it){
          if((*def_it) == (*old_it)){
            is_in_def = true;
            break;
          }
        }
        if(is_in_def == false){
          old_def.Append(*old_it);
        }
      }
    }
    else{
      for(auto old_it = old_out_list.begin();old_it != old_out_list.end();++old_it){
        old_def.Append(*old_it);
      }
    }
  }
  auto old_def_list = old_def.GetList();
  for(auto it = old_def_list.begin();it != old_def_list.end();++it){
    res->Append(*it);
  }
  if(use != nullptr){
    auto res_list = res->GetList();
    auto use_list = use->GetList();
    for(auto use_it = use_list.begin();use_it != use_list.end();++use_it){
      bool is_exist = false;
      for(auto res_it = res_list.begin();res_it != res_list.end();++res_it){
        if(*use_it == *res_it){
          is_exist = true;
          break;
        }
      }
      if(!is_exist){
        res->Append(*use_it);
      }
    }
  }
  
  return res;
}

temp::TempList* LiveGraphFactory::CalculateNewOut(fg::FNodePtr n){
  auto res = new temp::TempList();
  auto succ_list = n->Succ();
  auto succ_node_list = succ_list->GetList();
  for(auto node_it = succ_node_list.begin();node_it != succ_node_list.end();++node_it){
    auto succ_in = in_->Look(*node_it);
    if(succ_in == nullptr){
      // error
    }
    auto succ_in_list = succ_in->GetList();
    auto res_list = res->GetList();
    for(auto succ_it = succ_in_list.begin();succ_it != succ_in_list.end();++succ_it){
      bool is_exist = false;
      for(auto res_it = res_list.begin();res_it != res_list.end();++res_it){
        if(*res_it == *succ_it){
          is_exist = true;
          break;
        }
      }
      if(!is_exist){
        res->Append(*succ_it);
      }
    }
  }

  return res;
}

/** Maintain a data structure that remembers
 * what is live at the exit of each flow-graph node.
 * This function will create this data structure.
*/
void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  auto instr_node_list = flowgraph_->Nodes()->GetList();
  for(const auto &instr_node : instr_node_list){
    in_->Enter(instr_node, new temp::TempList());
    out_->Enter(instr_node, new temp::TempList());
  }
  while(true){
    bool is_same = true;
    for(const auto &instr_node : instr_node_list){
      auto node_in = in_->Look(instr_node);
      auto node_out = out_->Look(instr_node);
      if(node_in == nullptr || node_out == nullptr){
        // error
        return;
      }
      int old_in_set_size = node_in->GetList().size();
      int old_out_set_size = node_out->GetList().size();

      auto instr = instr_node->NodeInfo();
      auto instr_use = instr->Use();
      auto instr_def = instr->Def();
      auto new_in = CalculateNewIn(instr_use, node_out, instr_def);
      in_->Set(instr_node, new_in);
      delete node_in;
      auto new_out = CalculateNewOut(instr_node);
      out_->Set(instr_node, new_out);
      delete node_out;

      if(new_in->GetList().size() > old_in_set_size || new_out->GetList().size() > old_out_set_size){
        is_same = false;
      }
    }
    if(is_same){
      break;
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  auto registers = reg_manager->Registers();
  auto reg_list = registers->GetList();
  // precolored nodes
  for(auto reg : reg_list){
    auto reg_node = live_graph_.interf_graph->NewNode(reg);
    temp_node_map_->Enter(reg, reg_node);
  }
  for(auto reg1 : reg_list){
    for(auto reg2 : reg_list){
      if(reg1 != reg2){
        auto node1 = temp_node_map_->Look(reg1);
        auto node2 = temp_node_map_->Look(reg2);
        if(node1 == nullptr || node2 == nullptr){
          // error
          return;
        }
        live_graph_.interf_graph->AddEdge(node1, node2);
      }
    }
  }
  
  // add all temp node into graph
  auto instr_node_list = flowgraph_->Nodes()->GetList();
  for(auto instr_node : instr_node_list){
    auto out_set = out_->Look(instr_node);
    auto def_set = instr_node->NodeInfo()->Def();
    auto out_union_def = temp::TempList::UnionTempList(out_set, def_set);
    auto temp_list = out_union_def->GetList();
    for(auto temp : temp_list){
      if(temp == reg_manager->StackPointer()){
        // We shouldn't add %rsp into graph
        continue;
      }
      if(temp_node_map_->Look(temp) == nullptr){
        auto temp_node = live_graph_.interf_graph->NewNode(temp);
        temp_node_map_->Enter(temp, temp_node);
      }
    }
    delete out_union_def;
  }

  for(auto instr_node : instr_node_list){
    auto instr_info = instr_node->NodeInfo();
    auto out_set = out_->Look(instr_node);
    if(typeid(*instr_info) == typeid(assem::MoveInstr)){
      auto use_set = instr_info->Use();
      auto def_set = instr_info->Def();
      auto out_diff_use = temp::TempList::DiffTempList(out_set, use_set);
      auto out_diff_use_list = out_diff_use->GetList();
      if(def_set != nullptr){
        auto def_list = def_set->GetList();
        for(auto def : def_list){
          if(def == reg_manager->StackPointer()){
            continue;
          }
          for(auto live : out_diff_use_list){
            if(live == reg_manager->StackPointer()){
              continue;
            }
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(live));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(live), temp_node_map_->Look(def));
          }
          if(use_set != nullptr){
            auto use_list = use_set->GetList();
            for(auto use : use_list){
              if(use == reg_manager->StackPointer()){
                continue;
              }
              //!maybe wrong here
              if(!live_graph_.moves->Contain(temp_node_map_->Look(use), temp_node_map_->Look(def)) && !live_graph_.moves->Contain(temp_node_map_->Look(def), temp_node_map_->Look(use))){
                live_graph_.moves->Append(temp_node_map_->Look(def), temp_node_map_->Look(use));
              }
            }
          }
        }
      }
      delete out_diff_use;
    }
    else{
      auto def_set = instr_info->Def();
      if(def_set != nullptr){
        auto def_list = def_set->GetList();
        auto out_list = out_set->GetList();
        for(auto def : def_list){
          if(def == reg_manager->StackPointer()){
            continue;
          }
          for(auto out : out_list){
            if(out == reg_manager->StackPointer()){
              continue;
            }
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(out));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out), temp_node_map_->Look(def));
          }
        }
      }
    }
  }
  
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
