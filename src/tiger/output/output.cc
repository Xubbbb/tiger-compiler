#include "tiger/output/output.h"

#include <cstdio>
#include <regex>

#include "tiger/output/logger.h"
#include "tiger/runtime/gc/roots/roots.h"

extern frame::RegManager *reg_manager;
extern frame::Frags *frags;
std::vector<gc::AssemPointerMap> pointermap_list;

namespace output {
void AssemGen::GenAssem(bool need_ra) {
  frame::Frag::OutputPhase phase;

  // Output proc
  phase = frame::Frag::Proc;
  fprintf(out_, ".text\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  // Output string
  phase = frame::Frag::String;
  fprintf(out_, ".section .rodata\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  // Output pointermap
  fprintf(out_, ".global GLOBAL_GC_ROOTS\n");
  fprintf(out_, ".data\n");
  fprintf(out_, "GLOBAL_GC_ROOTS:\n");
  for(auto &&pointermap : pointermap_list) {
    fprintf(out_, "%s:\n", pointermap.label_.data());
    fprintf(out_, ".quad %s\n", pointermap.next_label_.data());
    fprintf(out_, ".quad %s\n", pointermap.key_label_.data());
    fprintf(out_, ".quad 0x%x\n", pointermap.framesize);
    for(auto offset : pointermap.root_offsets) {
      fprintf(out_, ".quad %d\n", offset);
    }
    fprintf(out_, ".quad %s\n", pointermap.endmap.data());
  }
}

} // namespace output

namespace frame {
/**
 * @brief Generate pointer map for a frame
 * @details We will find every "callq" instruction in this function and generate a pointer map for it
 * @param frame The frame to generate pointer map
 * @param fnode_list The flow graph of this function, we will use it to find the "callq" instruction
 * @param in_live The liveness analysis result of this function, we will use it to find the live registers before "callq" instruction
 * @param color The register allocation result of this function, we will use it to find the register name
*/
void generatePointerMap(frame::Frame *frame, fg::FNodeListPtr fnode_list, graph::Table<assem::Instr, temp::TempList> *in_live,temp::Map *color) {
  std::vector<int> root_offsets;
  std::vector<temp::Temp*> root_temps;
  std::vector<std::string> callee_save_list{"%rbx", "%rbp", "%r12", "%r13", "%r14", "%r15"};
  

  auto frame_locals = *(frame->locals);
  for(auto local_var : frame_locals) {
    if(local_var->is_pointer){
      if(typeid(*local_var) == typeid(frame::InFrameAccess)){
        auto frame_access = static_cast<frame::InFrameAccess*>(local_var);
        root_offsets.push_back(frame_access->offset);
      }
      else if(typeid(*local_var) == typeid(frame::InRegAccess)){
        auto reg_access = static_cast<frame::InRegAccess*>(local_var);
        root_temps.push_back(reg_access->reg);
      }
    }
  }

  std::vector<int> initialized_offsets;

  auto fnodes = fnode_list->GetList();
  for (auto fnode = fnodes.begin(); fnode != fnodes.end(); fnode++) {
    auto instr = (*fnode)->NodeInfo();
    if(typeid(*instr) == typeid(assem::OperInstr)){
      auto oper_instr = static_cast<assem::OperInstr*>(instr);
      auto assem_str = oper_instr->assem_;
      if(assem_str.substr(0, 5) == "callq"){
        auto next_fnode = std::next(fnode);
        auto next_label = (*next_fnode)->NodeInfo();
        if(typeid(*next_label) == typeid(assem::LabelInstr)){
          auto label_instr = static_cast<assem::LabelInstr*>(next_label);
          auto exceed_arg_num = temp::LabelFactory::LabelExceedArgNum(label_instr->label_);
          gc::AssemPointerMap pointermap;
          pointermap.label_ = "L" + label_instr->label_->Name();
          pointermap.key_label_ = label_instr->label_->Name();
          pointermap.framesize = (-frame->offset) + (frame->max_exceed_args + reg_manager->CalleeSaves()->GetList().size()) * reg_manager->WordSize();
          auto live_temps = in_live->Look(*fnode);
          for(int i = 0;i < root_temps.size();++i){
            bool is_live = false;
            for(auto live_temp : live_temps->GetList()){
              if(live_temp == root_temps[i]){
                is_live = true;
                break;
              }
            }
            if(is_live){
              for(int j = 0;j < callee_save_list.size();++j){
                if(color->Look(root_temps[i])->compare(callee_save_list[j]) == 0){
                  auto offset = (-pointermap.framesize) + (callee_save_list.size() - 1 - j + exceed_arg_num) * reg_manager->WordSize();
                  bool is_in_root_offsets = false;
                  for(auto root_offset : root_offsets){
                    if(root_offset == offset){
                      is_in_root_offsets = true;
                      break;
                    }
                  }
                  if(!is_in_root_offsets){
                    root_offsets.push_back(offset);
                  }
                  break;
                }
              }
            }
          }
          pointermap.root_offsets = initialized_offsets;
          pointermap.endmap = "1";
          if(pointermap_list.empty()){
            pointermap_list.push_back(pointermap);
          }
          else{
            pointermap_list.back().next_label_ = pointermap.label_;
            pointermap_list.push_back(pointermap);
          }
        }
      }
      /**
       * But how do we know a stack slot has been initialized
       * as a real pointer? I think we should do liveness analysis
       * for stack slot here. Here is just a naive implementation.
      */
      else if(assem_str.substr(0, 4) == "movq"){
        std::string framesize_label = frame->GetLabel() + "_framesize";
        // If assem_str contains framesize_label, it means this instruction is a stack slot operation
        if(assem_str.find(framesize_label) != std::string::npos){
          // We use regular expression to find the offset of the stack slot
          std::regex reg(framesize_label + "([+-]?[0-9]+)");
          std::smatch match;
          std::regex_search(assem_str, match, reg);
          int offset = 0;
          if(match.size() > 1){
            offset = std::stoi(match[1].str());
          }
          bool is_in_root_offsets = false;
          for(auto root_offset : root_offsets){
            if(root_offset == offset){
              is_in_root_offsets = true;
              break;
            }
          }
          if(is_in_root_offsets){
            bool is_in_initialized_offsets = false;
            for(auto initialized_offset : initialized_offsets){
              if(initialized_offset == offset){
                is_in_initialized_offsets = true;
                break;
              }
            }
            if(!is_in_initialized_offsets){
              initialized_offsets.push_back(offset);
            }
          }
        }
      }
    }
  }

  pointermap_list.back().next_label_ = "0";
}

void ProcFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  std::unique_ptr<canon::Traces> traces;
  std::unique_ptr<cg::AssemInstr> assem_instr;
  std::unique_ptr<ra::Result> allocation;

  // When generating proc fragment, do not output string assembly
  if (phase != Proc)
    return;

  TigerLog("-------====IR tree=====-----\n");
  TigerLog(body_);

  {
    // Canonicalize
    TigerLog("-------====Canonicalize=====-----\n");
    canon::Canon canon(body_);

    // Linearize to generate canonical trees
    TigerLog("-------====Linearlize=====-----\n");
    tree::StmList *stm_linearized = canon.Linearize();
    TigerLog(stm_linearized);

    // Group list into basic blocks
    TigerLog("------====Basic block_=====-------\n");
    canon::StmListList *stm_lists = canon.BasicBlocks();
    TigerLog(stm_lists);

    // Order basic blocks into traces_
    TigerLog("-------====Trace=====-----\n");
    tree::StmList *stm_traces = canon.TraceSchedule();
    TigerLog(stm_traces);

    traces = canon.TransferTraces();
  }

  temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
  {
    // Lab 5: code generation
    TigerLog("-------====Code generate=====-----\n");
    cg::CodeGen code_gen(frame_, std::move(traces));
    code_gen.Codegen();
    assem_instr = code_gen.TransferAssemInstr();
    TigerLog(assem_instr.get(), color);
  }

  assem::InstrList *il = assem_instr.get()->GetInstrList();
  
  if (need_ra) {
    // Lab 6: register allocation
    TigerLog("----====Register allocate====-----\n");
    ra::RegAllocator reg_allocator(frame_, std::move(assem_instr));
    reg_allocator.RegAlloc();
    allocation = reg_allocator.TransferResult();
    il = allocation->il_;
    color = temp::Map::LayerMap(reg_manager->temp_map_, allocation->coloring_);
    generatePointerMap(frame_, reg_allocator.color_->flow_fac->GetFlowGraph()->Nodes(), reg_allocator.color_->live_fac->in_.get(), color);
  }

  TigerLog("-------====Output assembly for %s=====-----\n",
           frame_->name_->Name().data());

  assem::Proc *proc = frame::ProcEntryExit3(frame_, il);
  
  std::string proc_name = frame_->GetLabel();

  fprintf(out, ".globl %s\n", proc_name.data());
  fprintf(out, ".type %s, @function\n", proc_name.data());
  // prologue
  fprintf(out, "%s", proc->prolog_.data());
  // body
  proc->body_->Print(out, color);
  // epilog_
  fprintf(out, "%s", proc->epilog_.data());
  fprintf(out, ".size %s, .-%s\n", proc_name.data(), proc_name.data());
}

void StringFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  // When generating string fragment, do not output proc assembly
  if (phase != String)
    return;

  fprintf(out, "%s:\n", label_->Name().data());
  int length = static_cast<int>(str_.size());
  // It may contain zeros in the middle of string. To keep this work, we need
  // to print all the charactors instead of using fprintf(str)
  fprintf(out, ".long %d\n", length);
  fprintf(out, ".string \"");
  for (int i = 0; i < length; i++) {
    if (str_[i] == '\n') {
      fprintf(out, "\\n");
    } else if (str_[i] == '\t') {
      fprintf(out, "\\t");
    } else if (str_[i] == '\"') {
      fprintf(out, "\\\"");
    } else {
      fprintf(out, "%c", str_[i]);
    }
  }
  fprintf(out, "\"\n");
}
} // namespace frame
