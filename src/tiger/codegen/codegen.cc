#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  // define fs name
  fs_ = frame_->GetLabel();
  auto instr_list = new assem::InstrList();
  auto stm_list = traces_->GetStmList()->GetList();
  for(auto stm : stm_list){
    stm->Munch(*instr_list, fs_);
  }
  auto instr_list_exit2 = frame::ProcEntryExit2(instr_list);
  assem_instr_ = std::make_unique<cg::AssemInstr>(instr_list_exit2);
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  assem::Instr* label_instr = new assem::LabelInstr(label_->Name(), label_);
  instr_list.Append(label_instr);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto jump_targets = new assem::Targets(jumps_);
  assem::Instr* jump_instr = new assem::OperInstr(
    "jmp `j0",
    nullptr,
    nullptr,
    jump_targets
  );
  instr_list.Append(jump_instr);
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left_temp = left_->Munch(instr_list, fs);
  auto right_temp = right_->Munch(instr_list, fs);
  auto cmpq_src = new temp::TempList();
  cmpq_src->Append(left_temp);
  cmpq_src->Append(right_temp);
  assem::Instr* cmpq_instr = new assem::OperInstr(
    "cmpq `s0,`s1",
    nullptr,
    cmpq_src,
    nullptr
  );

  std::string jump_instr_str = "";
  switch (op_)
  {
  case tree::RelOp::EQ_OP:
    jump_instr_str.append("je ");
    break;
  case tree::RelOp::NE_OP:
    jump_instr_str.append("jne ");
    break;
  //! Attention: Be careful about relation jump, instruction is opposite to semantic
  //! attention: dst_ = MemExp must arrange before
  case tree::RelOp::LT_OP:
    jump_instr_str.append("jg ");
    break;
  case tree::RelOp::GT_OP:
    jump_instr_str.append("jl ");
    break;
  case tree::RelOp::LE_OP:
    jump_instr_str.append("jge ");
    break;
  case tree::RelOp::GE_OP:
    jump_instr_str.append("jle ");
    break;
  default:
    break;
  }
  jump_instr_str.append("`j0");
  auto jump_label_list = new std::vector<temp::Label *>();
  jump_label_list->push_back(true_label_);
  jump_label_list->push_back(false_label_);
  auto jump_targets = new assem::Targets(jump_label_list);
  assem::Instr* jump_instr = new assem::OperInstr(
    jump_instr_str,
    nullptr,
    nullptr,
    jump_targets
  );

  instr_list.Append(cmpq_instr);
  instr_list.Append(jump_instr);
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //! attention: should be careful about the sequence of munch, we should munch dst firstly, src secondly
  //! attention: dst_ = MemExp must arrange before 
  if(typeid(*dst_) == typeid(tree::MemExp)){
    auto dst_mem_exp = static_cast<tree::MemExp *>(dst_);
    if(typeid(*(dst_mem_exp->exp_)) == typeid(tree::BinopExp)){
      auto dst_binop_exp = static_cast<tree::BinopExp *>(dst_mem_exp->exp_);
      if(dst_binop_exp->op_ == tree::BinOp::PLUS_OP){
        if(typeid(*src_) == typeid(tree::ConstExp)){
          auto src_const_exp = static_cast<tree::ConstExp *>(src_);
          if(typeid(*(dst_binop_exp->left_)) == typeid(tree::ConstExp)){
            /** movq $x, a(%rxx)
             *        ---move---
             *        |        |
             *      CONST     MEM
             *                 |
             *              ---+---
             *              |     |
             *            CONST
            */
            auto left_const_exp = static_cast<tree::ConstExp *>(dst_binop_exp->left_);
            auto right_temp = dst_binop_exp->right_->Munch(instr_list, fs); 
            std::string instr_assem = "movq $" + std::to_string(src_const_exp->consti_) + ", " + std::to_string(left_const_exp->consti_) + "(`s0)";
            auto src_list = new temp::TempList();
            src_list->Append(right_temp);
            assem::Instr* mov_instr = new assem::OperInstr(
              instr_assem,
              nullptr,
              src_list,
              nullptr
            );
            instr_list.Append(mov_instr);
            return;
          }
          else if(typeid(*(dst_binop_exp->right_)) == typeid(tree::ConstExp)){
            /** movq $x, a(%rxx)
             *        ---move---
             *        |        |
             *      CONST     MEM
             *                 |
             *              ---+---
             *              |     |
             *                  CONST
            */
            auto right_const_exp = static_cast<tree::ConstExp *>(dst_binop_exp->right_);
            auto left_temp = dst_binop_exp->left_->Munch(instr_list, fs);
            std::string instr_assem = "movq $" + std::to_string(src_const_exp->consti_) + ", " + std::to_string(right_const_exp->consti_) + "(`s0)";
            auto src_list = new temp::TempList();
            src_list->Append(left_temp);
            assem::Instr* mov_instr = new assem::OperInstr(
              instr_assem,
              nullptr,
              src_list,
              nullptr
            );
            instr_list.Append(mov_instr);
            return;
          }
        }
        else{
          if(typeid(*(dst_binop_exp->left_)) == typeid(tree::ConstExp)){
            /** movq %rxx, a(%rxx)
             *        ---move---
             *        |        |
             *                MEM
             *                 |
             *              ---+---
             *              |     |
             *            CONST
            */
            auto left_const_exp = static_cast<tree::ConstExp *>(dst_binop_exp->left_);
            auto right_temp = dst_binop_exp->right_->Munch(instr_list, fs);
            auto src_temp = src_->Munch(instr_list, fs);
            std::string instr_assem = "movq `s0, " + std::to_string(left_const_exp->consti_) + "(`s1)";
            auto src_list = new temp::TempList();
            src_list->Append(src_temp);
            src_list->Append(right_temp);
            assem::Instr* mov_instr = new assem::OperInstr(
              instr_assem,
              nullptr,
              src_list,
              nullptr
            );
            instr_list.Append(mov_instr);
            return;
          }
          else if(typeid(*(dst_binop_exp->right_)) == typeid(tree::ConstExp)){
            /** movq %rxx, a(%rxx)
             *        ---move---
             *        |        |
             *                MEM
             *                 |
             *              ---+---
             *              |     |
             *                  CONST
            */
            auto right_const_exp = static_cast<tree::ConstExp *>(dst_binop_exp->right_);
            auto left_temp = dst_binop_exp->left_->Munch(instr_list, fs);
            auto src_temp = src_->Munch(instr_list, fs);
            std::string instr_assem = "movq `s0, " + std::to_string(right_const_exp->consti_) + "(`s1)";
            auto src_list = new temp::TempList();
            src_list->Append(src_temp);
            src_list->Append(left_temp);
            assem::Instr* mov_instr = new assem::OperInstr(
              instr_assem,
              nullptr,
              src_list,
              nullptr
            );
            instr_list.Append(mov_instr);
            return;
          }
        }
      }
    }
  }

  if(typeid(*dst_) == typeid(tree::MemExp) && typeid(*src_) == typeid(tree::ConstExp)){
    /** movq $x, (%rxx)
     *        ---move---
     *        |        |
     *      CONST     MEM
     *                 |
    */
    auto src_const_exp = static_cast<tree::ConstExp *>(src_);
    auto dst_mem_exp = static_cast<tree::MemExp *>(dst_);
    auto dst_temp = dst_mem_exp->exp_->Munch(instr_list, fs);
    std::string instr_assem = "movq $" + std::to_string(src_const_exp->consti_) + ", (`s0)";
    auto src_list = new temp::TempList();
    src_list->Append(dst_temp);
    assem::Instr* mov_instr = new assem::OperInstr(
      instr_assem,
      nullptr,
      src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    return;
  }

  if(typeid(*dst_) == typeid(tree::MemExp)){
    /** movq %rxx, (%rxx)
     *        ---move---
     *        |        |
     *                MEM     
    */
    auto dst_mem_exp = static_cast<tree::MemExp *>(dst_);
    auto dst_temp = dst_mem_exp->exp_->Munch(instr_list, fs);
    auto src_temp = src_->Munch(instr_list, fs);
    std::string instr_assem = "movq `s0, (`s1)";
    auto src_list = new temp::TempList();
    src_list->Append(src_temp);
    src_list->Append(dst_temp);
    assem::Instr* mov_instr = new assem::OperInstr(
      instr_assem,
      nullptr,
      src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    return;
  }


  if(typeid(*src_) == typeid(tree::MemExp)){
    auto src_mem_exp = static_cast<tree::MemExp *>(src_);
    if(typeid(*(src_mem_exp->exp_)) == typeid(tree::BinopExp)){
      auto src_binop_exp = static_cast<tree::BinopExp *>(src_mem_exp->exp_);
      if(src_binop_exp->op_ == tree::BinOp::PLUS_OP){
        if(typeid(*(src_binop_exp->left_)) == typeid(tree::ConstExp)){
          /** movq a(%rxx), %rxx
           *        ---move---
           *        |
           *       MEM
           *        |
           *     ---+---
           *     |     |
           *   CONST
          */
          auto dst_temp = dst_->Munch(instr_list, fs);
          auto left_const_exp = static_cast<tree::ConstExp *>(src_binop_exp->left_);
          auto right_temp = src_binop_exp->right_->Munch(instr_list, fs);
          std::string instr_assem = "movq " + std::to_string(left_const_exp->consti_) + "(`s0), `d0";
          auto dst_list = new temp::TempList();
          dst_list->Append(dst_temp);
          auto src_list = new temp::TempList();
          src_list->Append(right_temp);
          assem::Instr* mov_instr = new assem::OperInstr(
            instr_assem,
            dst_list,
            src_list,
            nullptr
          );
          instr_list.Append(mov_instr);
          return;
        }
        else if(typeid(*(src_binop_exp->right_)) == typeid(tree::ConstExp)){
          /** movq a(%rxx), %rxx
           *        ---move---
           *        |
           *       MEM
           *        |
           *     ---+---
           *     |     |
           *         CONST
          */
          auto dst_temp = dst_->Munch(instr_list, fs);
          auto right_const_exp = static_cast<tree::ConstExp *>(src_binop_exp->right_);
          auto left_temp = src_binop_exp->left_->Munch(instr_list, fs);
          std::string instr_assem = "movq " + std::to_string(right_const_exp->consti_) + "(`s0), `d0";
          auto dst_list = new temp::TempList();
          dst_list->Append(dst_temp);
          auto src_list = new temp::TempList();
          src_list->Append(left_temp);
          assem::Instr* mov_instr = new assem::OperInstr(
            instr_assem,
            dst_list,
            src_list,
            nullptr
          );
          instr_list.Append(mov_instr);
          return;
        }
      }
    }
  }

  if(typeid(*src_) == typeid(tree::ConstExp)){
    /** movq $x, %rxx
     *        ---move---
     *        |        |
     *      CONST     
    */
    auto src_const_exp = static_cast<tree::ConstExp *>(src_);
    auto dst_temp = dst_->Munch(instr_list, fs);
    std::string instr_assem = "movq $" + std::to_string(src_const_exp->consti_) + ", `d0";
    auto dst_list = new temp::TempList();
    dst_list->Append(dst_temp);
    assem::Instr* mov_instr = new assem::OperInstr(
      instr_assem,
      dst_list,
      nullptr,
      nullptr
    );
    instr_list.Append(mov_instr);
    return;
  }

  if(typeid(*src_) == typeid(tree::MemExp)){
    /** movq (%rxx), %rxx
     *        ---move---
     *        |        |
     *       MEM     
    */
    auto dst_temp = dst_->Munch(instr_list, fs); 
    auto src_mem_exp = static_cast<tree::MemExp *>(src_);
    auto src_temp = src_mem_exp->exp_->Munch(instr_list, fs);
    std::string instr_assem = "movq (`s0), `d0";
    auto dst_list = new temp::TempList();
    dst_list->Append(dst_temp);
    auto src_list = new temp::TempList();
    src_list->Append(src_temp);
    assem::Instr* mov_instr = new assem::OperInstr(
      instr_assem,
      dst_list,
      src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    return;
  }

  /** movq %rxx, %rxx
   *        ---move---
   *        |        |     
  */
  auto dst_temp = dst_->Munch(instr_list, fs); 
  auto src_temp = src_->Munch(instr_list, fs);
  std::string instr_assem = "movq `s0, `d0";
  auto dst_list = new temp::TempList();
  dst_list->Append(dst_temp);
  auto src_list = new temp::TempList();
  src_list->Append(src_temp);
  assem::Instr* mov_instr = new assem::MoveInstr(
    instr_assem,
    dst_list,
    src_list
  );
  instr_list.Append(mov_instr);
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //! Temp's is_pointer may be wrong
  switch (op_)
  {
  case tree::BinOp::PLUS_OP:
  { 
    if(typeid(*left_) == typeid(tree::ConstExp)){
      /** movq %rxx1, %rxx2
       *  addq $x, %rxx2
       * or just use "leaq x(%rxx1), %rxx2"
       *      ---+---
       *      |     |
       *    CONST
      */
      auto left_const_exp = static_cast<tree::ConstExp *>(left_);
      auto right_temp = right_->Munch(instr_list, fs);
      auto result_reg = temp::TempFactory::NewTemp(right_temp->is_pointer);
      std::string instr_assem = "leaq " + std::to_string(left_const_exp->consti_) + "(`s0), `d0";
      auto dst_list = new temp::TempList();
      dst_list->Append(result_reg);
      auto src_list = new temp::TempList();
      src_list->Append(right_temp);
      assem::Instr* lea_instr = new assem::OperInstr(
        instr_assem,
        dst_list,
        src_list,
        nullptr
      );
      instr_list.Append(lea_instr);
      return result_reg;
    }
    else if(typeid(*right_) == typeid(tree::ConstExp)){
      /** movq %rxx1, %rxx2
       *  addq $x, %rxx2
       * or just use "leaq x(%rxx1), %rxx2"
       *      ---+---
       *      |     |
       *          CONST
      */
      auto right_const_exp = static_cast<tree::ConstExp *>(right_);
      auto left_temp = left_->Munch(instr_list, fs);
      auto result_reg = temp::TempFactory::NewTemp(left_temp->is_pointer);
      std::string instr_assem = "leaq " + std::to_string(right_const_exp->consti_) + "(`s0), `d0";
      auto dst_list = new temp::TempList();
      dst_list->Append(result_reg);
      auto src_list = new temp::TempList();
      src_list->Append(left_temp);
      assem::Instr* lea_instr = new assem::OperInstr(
        instr_assem,
        dst_list,
        src_list,
        nullptr
      );
      instr_list.Append(lea_instr);
      return result_reg;
    }
    /** movq %rxx1, %rxx3  addq %rxx2, %rxx3
     *    ---+---
     *    |     |
    */
    auto left_temp = left_->Munch(instr_list, fs);
    auto right_temp = right_->Munch(instr_list, fs);
    auto result_reg = temp::TempFactory::NewTemp(left_temp->is_pointer || right_temp->is_pointer);

    std::string mov_assem = "movq `s0, `d0";
    auto mov_dst_list = new temp::TempList();
    mov_dst_list->Append(result_reg);
    auto mov_src_list = new temp::TempList();
    mov_src_list->Append(left_temp);
    assem::Instr* mov_instr = new assem::MoveInstr(
      mov_assem,
      mov_dst_list,
      mov_src_list
    );
    std::string add_assem = "addq `s0, `d0";
    auto add_dst_list = new temp::TempList();
    add_dst_list->Append(result_reg);
    auto add_src_list = new temp::TempList();
    add_src_list->Append(right_temp);add_src_list->Append(result_reg);
    assem::Instr* add_instr = new assem::OperInstr(
      add_assem,
      add_dst_list,
      add_src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    instr_list.Append(add_instr);
    return result_reg;
    break;
  }
  case tree::BinOp::MINUS_OP:
  {
    auto left_temp = left_->Munch(instr_list, fs);
    auto right_temp = right_->Munch(instr_list, fs);
    auto result_reg = temp::TempFactory::NewTemp(left_temp->is_pointer || right_temp->is_pointer);

    std::string mov_assem = "movq `s0, `d0";
    auto mov_dst_list = new temp::TempList();
    mov_dst_list->Append(result_reg);
    auto mov_src_list = new temp::TempList();
    mov_src_list->Append(left_temp);
    assem::Instr* mov_instr = new assem::MoveInstr(
      mov_assem,
      mov_dst_list,
      mov_src_list
    );
    std::string sub_assem = "subq `s0, `d0";
    auto sub_dst_list = new temp::TempList();
    sub_dst_list->Append(result_reg);
    auto sub_src_list = new temp::TempList();
    sub_src_list->Append(right_temp);sub_src_list->Append(result_reg);
    assem::Instr* sub_instr = new assem::OperInstr(
      sub_assem,
      sub_dst_list,
      sub_src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    instr_list.Append(sub_instr);
    return result_reg;
    break;
  }
  case tree::BinOp::MUL_OP:
  { 
    auto left_temp = left_->Munch(instr_list, fs);
    auto right_temp = right_->Munch(instr_list, fs);

    auto rax = reg_manager->GetRegister(0);
    auto rdx = reg_manager->GetRegister(3);
    std::string mov_assem = "movq `s0, `d0";
    auto mov_dst_list = new temp::TempList();
    mov_dst_list->Append(rax);
    auto mov_src_list = new temp::TempList();
    mov_src_list->Append(left_temp);
    assem::Instr* mov_instr = new assem::MoveInstr(
      mov_assem,
      mov_dst_list,
      mov_src_list
    );
    std::string mul_assem = "imulq `s0";
    auto mul_dst_list = new temp::TempList();
    mul_dst_list->Append(rax);
    mul_dst_list->Append(rdx);
    auto mul_src_list = new temp::TempList();
    mul_src_list->Append(right_temp);mul_src_list->Append(rax);
    assem::Instr* mul_instr = new assem::OperInstr(
      mul_assem,
      mul_dst_list,
      mul_src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    instr_list.Append(mul_instr);
    return rax;
    break;
  }
  case tree::BinOp::DIV_OP:
  { 
    auto left_temp = left_->Munch(instr_list, fs);
    auto right_temp = right_->Munch(instr_list, fs);

    auto rax = reg_manager->GetRegister(0);
    auto rdx = reg_manager->GetRegister(3);
    std::string mov_assem = "movq `s0, `d0";
    auto mov_dst_list = new temp::TempList();
    mov_dst_list->Append(rax);
    auto mov_src_list = new temp::TempList();
    mov_src_list->Append(left_temp);
    assem::Instr* mov_instr = new assem::MoveInstr(
      mov_assem,
      mov_dst_list,
      mov_src_list
    );
    std::string cqto_assem = "cqto";
    assem::Instr* cqto_instr = new assem::OperInstr(
      cqto_assem,
      new temp::TempList(rdx),
      nullptr,
      nullptr
    );
    std::string div_assem = "idivq `s0";
    auto div_dst_list = new temp::TempList();
    div_dst_list->Append(rax);
    div_dst_list->Append(rdx);
    auto div_src_list = new temp::TempList();
    div_src_list->Append(right_temp);div_src_list->Append(rax);div_src_list->Append(rdx);
    assem::Instr* div_instr = new assem::OperInstr(
      div_assem,
      div_dst_list,
      div_src_list,
      nullptr
    );
    instr_list.Append(mov_instr);
    instr_list.Append(cqto_instr);
    instr_list.Append(div_instr);
    return rax;
    break;
  }
  default:
    break;
  }
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*exp_) == typeid(tree::BinopExp)){
    auto binop_exp = static_cast<tree::BinopExp *>(exp_);
    if(binop_exp->op_ == tree::BinOp::PLUS_OP){
      if(typeid(*(binop_exp->left_)) == typeid(tree::ConstExp)){
        /**
         *     MEM
         *      |
         *   ---+----
         *   |      |
         * CONST
        */
        auto left_const_exp = static_cast<tree::ConstExp *>(binop_exp->left_);
        auto result_reg = temp::TempFactory::NewTemp(false);
        auto right_temp = binop_exp->right_->Munch(instr_list, fs);
        std::string instr_assem = "movq " + std::to_string(left_const_exp->consti_) + "(`s0), `d0";
        auto dst_list = new temp::TempList();
        dst_list->Append(result_reg);
        auto src_list = new temp::TempList();
        src_list->Append(right_temp);
        assem::Instr* mov_instr = new assem::OperInstr(
          instr_assem,
          dst_list,
          src_list,
          nullptr
        );
        instr_list.Append(mov_instr);
        return result_reg;
      }
      else if(typeid(*(binop_exp->right_)) == typeid(tree::ConstExp)){
        /**
         *     MEM
         *      |
         *   ---+----
         *   |      |
         *        CONST
        */
        auto right_const_exp = static_cast<tree::ConstExp *>(binop_exp->right_);
        auto result_reg = temp::TempFactory::NewTemp(false);
        auto left_temp = binop_exp->left_->Munch(instr_list, fs);
        std::string instr_assem = "movq " + std::to_string(right_const_exp->consti_) + "(`s0), `d0";
        auto dst_list = new temp::TempList();
        dst_list->Append(result_reg);
        auto src_list = new temp::TempList();
        src_list->Append(left_temp);
        assem::Instr* mov_instr = new assem::OperInstr(
          instr_assem,
          dst_list,
          src_list,
          nullptr
        );
        instr_list.Append(mov_instr);
        return result_reg;
      }
    }
  }

  /**
   *   MEM
   *    |
  */
  auto result_reg = temp::TempFactory::NewTemp(false);
  auto src_temp = exp_->Munch(instr_list, fs);
  std::string instr_assem = "movq (`s0), `d0";
  auto dst_list = new temp::TempList();
  dst_list->Append(result_reg);
  auto src_list = new temp::TempList();
  src_list->Append(src_temp);

  assem::Instr* mov_instr = new assem::OperInstr(
    instr_assem,
    dst_list,
    src_list,
    nullptr
  );

  instr_list.Append(mov_instr);
  return result_reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // if we want to use FP we should calculate(SP + k + fs).In assemble, we generate label_framesize(%rsp).
  if(temp_ == reg_manager->FramePointer()){
    auto result_reg = temp::TempFactory::NewTemp(false);
    std::string leaq_assem = "";
    leaq_assem.append("leaq ");
    leaq_assem.append(std::string(fs));
    leaq_assem.append("_framesize(`s0), `d0");
    auto src_list = new temp::TempList();
    auto dst_list = new temp::TempList();
    src_list->Append(reg_manager->StackPointer());
    dst_list->Append(result_reg);
    assem::Instr* leaq_instr = new assem::OperInstr(
      leaq_assem,
      dst_list,
      src_list,
      nullptr
    );
    instr_list.Append(leaq_instr);
    return result_reg;
  }
  // Otherwise, we only return this register itself.
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  /**
   * use 'leaq label(%rip), temp' to represent move the addr of label into temp
  */
  auto result_reg = temp::TempFactory::NewTemp(false);
  std::string leaq_assem = "";
  leaq_assem.append("leaq ");
  leaq_assem.append(name_->Name());
  leaq_assem.append("(%rip),`d0");
  auto dst_list = new temp::TempList();
  dst_list->Append(result_reg);
  assem::Instr* leaq_instr = new assem::OperInstr(
    leaq_assem,
    dst_list,
    nullptr,
    nullptr
  );
  instr_list.Append(leaq_instr);
  return result_reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto result_reg = temp::TempFactory::NewTemp(false);
  std::string mov_assem = "movq $" + std::to_string(consti_) + ", `d0";
  auto dst_list = new temp::TempList();
  dst_list->Append(result_reg);
  assem::Instr* mov_instr = new assem::OperInstr(
    mov_assem,
    dst_list,
    nullptr,
    nullptr
  );
  instr_list.Append(mov_instr);
  return result_reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto arg_list = args_->GetList();
  int exceed_num = static_cast<int>(arg_list.size()) - static_cast<int>(reg_manager->ArgRegs()->GetList().size());
  if(exceed_num > 0){
    // This means that we should allocate space on stack to pass some args
    std::string alloc_assem = "subq $" + std::to_string(exceed_num * reg_manager->WordSize()) + ", `d0";
    auto dst_list = new temp::TempList();
    dst_list->Append(reg_manager->StackPointer());
    auto src_list = new temp::TempList();
    src_list->Append(reg_manager->StackPointer());
    assem::Instr* alloc_instr = new assem::OperInstr(
      alloc_assem,
      dst_list,
      src_list,
      nullptr
    );
    instr_list.Append(alloc_instr);
  }
  auto calldefs_list = reg_manager->CallerSaves();
  calldefs_list->Append(reg_manager->ReturnValue());
  auto src_list = args_->MunchArgs(instr_list, fs);
  if(typeid(*fun_) == typeid(tree::NameExp)){
    auto fun_name_exp = static_cast<tree::NameExp *>(fun_);
    auto fun_name_str = fun_name_exp->name_->Name();
    std::string call_assem = "callq " + fun_name_str;
    assem::Instr* call_instr = new assem::OperInstr(
      call_assem,
      calldefs_list,
      src_list,
      nullptr
    );
    instr_list.Append(call_instr);
  }
  else{
    return reg_manager->ReturnValue();
  }
  /**
   * GC : We put a label at the end of this call
   * then pointer map will point to this label(which is also this call's return address)
  */
  auto pointer_map_label = temp::LabelFactory::NewLabel();
  instr_list.Append(new assem::LabelInstr(pointer_map_label->Name(), pointer_map_label));

  if(exceed_num > 0){
    // If num of args exceed, we should dealloc those args on stack
    std::string dealloc_assem = "addq $" + std::to_string(exceed_num * reg_manager->WordSize()) + ", `d0";
    auto dst_list = new temp::TempList();
    dst_list->Append(reg_manager->StackPointer());
    auto src_list = new temp::TempList();
    src_list->Append(reg_manager->StackPointer());
    assem::Instr* dealloc_instr = new assem::OperInstr(
      dealloc_assem,
      dst_list,
      src_list,
      nullptr
    );
    instr_list.Append(dealloc_instr);
  }
  return reg_manager->ReturnValue();
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto arg_reg_temp_list = reg_manager->ArgRegs();
  const int MAX_ARG_REGS = arg_reg_temp_list->GetList().size();
  const int WORD_SIZE = reg_manager->WordSize();

  auto res_temp_list = new temp::TempList();
  auto exp_it = exp_list_.begin();
  for(int i = 0;exp_it != exp_list_.end();++i, ++exp_it){
    auto arg_temp = (*exp_it)->Munch(instr_list, fs);
    if(i < MAX_ARG_REGS){
      // This arg can be put into register
      std::string mov_assem = "movq `s0, `d0";
      auto dst_list = new temp::TempList();
      dst_list->Append(arg_reg_temp_list->NthTemp(i));
      auto src_list = new temp::TempList();
      src_list->Append(arg_temp);
      assem::Instr* mov_instr = new assem::MoveInstr(
        mov_assem,
        dst_list,
        src_list
      );
      instr_list.Append(mov_instr);
      res_temp_list->Append(arg_reg_temp_list->NthTemp(i));
    }
    else{
      // Push other args from right to left in order
      //? But what about the sequence of calculating args? Maybe in canonical, we have pull all CALLEXP to the top?
      std::string mov_assem = "movq `s0, " + std::to_string((i - MAX_ARG_REGS) * WORD_SIZE) + "(`s1)";
      auto src_list = new temp::TempList();
      src_list->Append(arg_temp);
      src_list->Append(reg_manager->StackPointer());
      assem::Instr* mov_instr = new assem::OperInstr(
        mov_assem,
        nullptr,
        src_list,
        nullptr
      );
      instr_list.Append(mov_instr);
    }
  }

  return res_temp_list;
}

} // namespace tree
