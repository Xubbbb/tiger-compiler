#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
tree::Exp* InFrameAccess::ToExp(tree::Exp* framePtr) const {
  tree::BinopExp* bin_op_exp = new tree::BinopExp(tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset));
  return new tree::MemExp(bin_op_exp);
}

tree::Exp* InRegAccess::ToExp(tree::Exp* framePtr) const {
  return new tree::TempExp(reg);
}

/* TODO: Put your lab5 code here */

X64RegManager::X64RegManager(){
  // init register
  for(int i = 0;i < 16;++i){
    auto reg_temp = temp::TempFactory::NewTemp();
    regs_.push_back(reg_temp);
    std::string* register_name = nullptr;
    switch (i)
    {
      case 0:
        register_name = new std::string("%rax");
        break;
      case 1:
        register_name = new std::string("%rbx");
        break;
      case 2:
        register_name = new std::string("%rcx");
        break;
      case 3:
        register_name = new std::string("%rdx");
        break;
      case 4:
        register_name = new std::string("%rsi");
        break;
      case 5:
        register_name = new std::string("%rdi");
        break;
      case 6:
        register_name = new std::string("%rbp");
        break;
      case 7:
        register_name = new std::string("%rsp");
        break;
      case 8:
        register_name = new std::string("%r8");
        break;
      case 9:
        register_name = new std::string("%r9");
        break;
      case 10:
        register_name = new std::string("%r10");
        break;
      case 11:
        register_name = new std::string("%r11");
        break;
      case 12:
        register_name = new std::string("%r12");
        break;
      case 13:
        register_name = new std::string("%r13");
        break;
      case 14:
        register_name = new std::string("%r14");
        break;
      case 15:
        register_name = new std::string("%r15");
        break;
      default:
        break;
    }
    temp_map_->Enter(reg_temp, register_name);
  }
}

temp::TempList *X64RegManager::Registers() {
  return nullptr;
}

temp::TempList *X64RegManager::ArgRegs() {
  auto arg_regs = new temp::TempList();
  arg_regs->Append(regs_[5]);
  arg_regs->Append(regs_[4]);
  arg_regs->Append(regs_[3]);
  arg_regs->Append(regs_[2]);
  arg_regs->Append(regs_[8]);
  arg_regs->Append(regs_[9]);
  return arg_regs;
}

temp::TempList *X64RegManager::CallerSaves() {
  auto caller_regs = new temp::TempList();
  caller_regs->Append(regs_[0]);
  caller_regs->Append(regs_[2]);
  caller_regs->Append(regs_[3]);
  caller_regs->Append(regs_[4]);
  caller_regs->Append(regs_[5]);
  caller_regs->Append(regs_[8]);
  caller_regs->Append(regs_[9]);
  caller_regs->Append(regs_[10]);
  caller_regs->Append(regs_[11]);
  return caller_regs;
}

temp::TempList *X64RegManager::CalleeSaves() {
  auto callee_regs = new temp::TempList();
  callee_regs->Append(regs_[1]);
  callee_regs->Append(regs_[6]);
  callee_regs->Append(regs_[12]);
  callee_regs->Append(regs_[13]);
  callee_regs->Append(regs_[14]);
  callee_regs->Append(regs_[15]);
  return callee_regs;
}

temp::TempList *X64RegManager::ReturnSink() {
  auto temp_list = CalleeSaves();
  temp_list->Append(StackPointer());
  temp_list->Append(ReturnValue());
  return temp_list;
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_[6];
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_[7];
}

temp::Temp *X64RegManager::ReturnValue() {
  return regs_[0];
}

X64Frame::X64Frame(temp::Label* name, std::list<bool>* formals)
  :Frame(name, formals)
{ 
  offset = 0;
  if(formals != nullptr){
    this->formals = new std::list<frame::Access *>();
    for(const auto is_escape : (*formals)){
      this->formals->push_back(AllocLocal(is_escape));
    }
  }
}

frame::Access* X64Frame::AllocLocal(bool escape){
  Access* new_access = nullptr;
  if(escape){
    offset -= reg_manager->WordSize();
    new_access = new InFrameAccess(offset);
  }
  else{
    new_access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  return new_access;
}

tree::Exp* ExternalCall(std::string s, tree::ExpList* args){
  return new tree::CallExp(
    new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
    args
  );
}

frame::ProcFrag* ProcEntryExit1(frame::Frame* frame_, tree::Stm* stm_){
  // Receive formal
  auto formal_list_ptr = frame_->Formals();
  tree::Stm* stm_receive_param = nullptr;
  auto ArgRegs = reg_manager->ArgRegs();
  int i = 0;
  if(formal_list_ptr != nullptr){
    for(auto formal_access : (*formal_list_ptr)){
      if(i < 6){
        if(stm_receive_param == nullptr){
          stm_receive_param = new tree::MoveStm(
            formal_access->ToExp(new tree::TempExp(reg_manager->FramePointer())),
            new tree::TempExp(ArgRegs->NthTemp(i))
          );
        }
        else{
          stm_receive_param = new tree::SeqStm(
            stm_receive_param,
            new tree::MoveStm(
              formal_access->ToExp(new tree::TempExp(reg_manager->FramePointer())),
              new tree::TempExp(ArgRegs->NthTemp(i))
            )
          );
        }
      }
      else{
        stm_receive_param = new tree::SeqStm(
          stm_receive_param,
          new tree::MoveStm(
            formal_access->ToExp(new tree::TempExp(reg_manager->FramePointer())),
            new tree::MemExp(
              new tree::BinopExp(
                tree::BinOp::PLUS_OP,
                new tree::TempExp(reg_manager->FramePointer()),
                //!Attention: don't forget return address is also on the stack, we should skip it
                new tree::ConstExp(
                  (i - 5) * reg_manager->WordSize()
                )
              )
            )
          )
        );
      }
      ++i;
    }
  }

  // callee save
  //! There is a problem here:
  //! %rbp will be interepted as _framesize(%rsp) in codegen
  //! but this is ok in lab5, in lab6 if we want to truely save
  //! %rbp in lab6 maybe we should use some tricks such as 
  //! regard FramePointer as not %rbp use another temp replacement in this phase
  auto callee_save_regs = reg_manager->CalleeSaves();
  tree::Stm* stm_callee_save = nullptr;
  auto num_callee_save = callee_save_regs->GetList().size();
  std::vector<temp::Temp*> saving_list;
  saving_list.clear();
  for(int j = 0;j < num_callee_save;++j){
    auto new_saving_reg = temp::TempFactory::NewTemp();
    saving_list.push_back(new_saving_reg);
    if(stm_callee_save == nullptr){
      stm_callee_save = new tree::MoveStm(
        new tree::TempExp(new_saving_reg),
        new tree::TempExp(callee_save_regs->NthTemp(j))
      );
    }
    else{
      stm_callee_save = new tree::SeqStm(
        stm_callee_save,
        new tree::MoveStm(
          new tree::TempExp(new_saving_reg),
          new tree::TempExp(callee_save_regs->NthTemp(j))
        )
      );
    }
  }

  // callee save restore
  tree::Stm* stm_callee_save_restore = nullptr;
  for(int j = 0;j < num_callee_save;++j){
    if(stm_callee_save_restore == nullptr){
      stm_callee_save_restore = new tree::MoveStm(
        new tree::TempExp(callee_save_regs->NthTemp(j)),
        new tree::TempExp(saving_list[j])
      );
    }
    else{
      stm_callee_save_restore = new tree::SeqStm(
        stm_callee_save_restore,
        new tree::MoveStm(
          new tree::TempExp(callee_save_regs->NthTemp(j)),
          new tree::TempExp(saving_list[j])
        )
      );
    }
  }


  //? What about sequence of view_shift and callee save?
  //! We try view shift first and then callee save just as the sequence on the book
  // todo: maybe change the order
  if(formal_list_ptr != nullptr){
    return new ProcFrag(
      new tree::SeqStm(
        stm_receive_param,
        new tree::SeqStm(
          stm_callee_save,
          new tree::SeqStm(
            stm_,
            stm_callee_save_restore
          )
        )
      ),
      frame_
    );
  }
  else{
    return new ProcFrag(
      new tree::SeqStm(
        stm_callee_save,
        new tree::SeqStm(
          stm_,
          stm_callee_save_restore
        )
      ),
      frame_
    );
  }
}

assem::InstrList* ProcEntryExit2(assem::InstrList * body){
  body->Append(
    new assem::OperInstr(
      "",
      new temp::TempList(),
      reg_manager->ReturnSink(),
      nullptr
    )
  );
  return body;
}

assem::Proc* ProcEntryExit3(frame::Frame* frame, assem::InstrList* body){
  std::string prologue = ".set " + frame->GetLabel() + "_framesize, " + std::to_string(-(frame->offset)) + "\n";
  prologue += frame->GetLabel() + ":\n";
  prologue += "subq $" + std::to_string(-(frame->offset)) + ", %rsp\n";
  std::string epilogue = "addq $" + std::to_string(-(frame->offset)) + ", %rsp\n";
  epilogue += "retq\n";
  return new assem::Proc(prologue, body, epilogue);
}

} // namespace frame
