#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include <iostream>

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

/**
 * FindFP will find the real frame of var level by level
*/
tree::Exp* FindFP(Level* end, Level* start){
  // The basic situation is that this var is not escaped, current fp is this var's fp, return a TempExp(FP) is enough
  tree::Exp* res_exp = new tree::TempExp(reg_manager->FramePointer());
  auto level_it = start;
  while(level_it != end){
    // The first formal of a frame is the parent frame's fp
    auto static_link_access = level_it->frame_->Formals()->front();
    // current res_exp is current frame's fp
    res_exp = static_link_access->ToExp(res_exp);
    level_it = level_it->parent_;
  }
  return res_exp;
}

Level::Level(tr::Level *parent, temp::Label *name, std::list<bool> *formals, std::list<bool> *is_pointer)
  :parent_(parent),
  frame_(nullptr){
    if(formals != nullptr){
      formals->push_front(true);
    }
    if(is_pointer != nullptr){
      is_pointer->push_front(false);
    }
    frame_ = new frame::X64Frame(name, formals, is_pointer);
}

Access *Access::AllocLocal(Level *level, bool escape, bool is_pointer) {
  /* TODO: Put your lab5 code here */
  auto new_access = level->frame_->AllocLocal(escape, is_pointer);
  return new tr::Access(level, new_access);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    tree::CjumpStm* stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    std::list<temp::Label **> trues_patch_list;
    std::list<temp::Label **> falses_patch_list;
    trues_patch_list.clear();
    trues_patch_list.push_back(&stm->true_label_);
    falses_patch_list.clear();
    falses_patch_list.push_back(&stm->false_label_);
    PatchList trues = PatchList(trues_patch_list);
    PatchList falses = PatchList(falses_patch_list);
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(errormsg->GetTokPos(), "Can't convert a Nx to a Cx!");
    PatchList fake_patch_list;
    return Cx(fake_patch_list, fake_patch_list, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp* r = temp::TempFactory::NewTemp(false);
    temp::Label* t = temp::LabelFactory::NewLabel();
    temp::Label* f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
      new tree::EseqExp(
        cx_.stm_,
        new tree::EseqExp(
          new tree::LabelStm(f),
          new tree::EseqExp(
            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0)),
            new tree::EseqExp(
              new tree::LabelStm(t),
              new tree::TempExp(r)
            )
          )
        )
      )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    tree::Exp* un_ex_res = UnEx();
    return new tree::ExpStm(un_ex_res);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  auto main_exp_ty = absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), nullptr, errormsg_.get());
  frags->PushBack(frame::ProcEntryExit1(main_level_->frame_, main_exp_ty->exp_->UnNx()));
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp_res = nullptr;
  type::Ty* ty_res = nullptr;
  env::EnvEntry* entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)){
    auto var_entry = static_cast<env::VarEntry*>(entry);
    auto var_level = var_entry->access_->level_;
    auto real_fp = tr::FindFP(var_level, level);
    auto mem_exp = var_entry->access_->access_->ToExp(real_fp);
    exp_res = new tr::ExExp(mem_exp);
    ty_res = var_entry->ty_->ActualTy();
  }
  else{
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }

  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp_res = nullptr;
  type::Ty* ty_res = nullptr;
  // First, we will get this record var's position
  // record_exp_ty.exp_ is a ExExp(MemExp)
  auto record_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  // Second, we should get this field's order in record var's field list
  const auto record_field_list = (static_cast<type::RecordTy*>(record_exp_ty->ty_))->fields_->GetList();
  int field_order = 0;
  for(const auto field : record_field_list){
    if(field->name_ == sym_){
      ty_res = field->ty_->ActualTy();
      break;
    }
    ++field_order;
  }
  const int field_offset = field_order * reg_manager->WordSize();
  auto bin_op_exp = new tree::BinopExp(tree::BinOp::PLUS_OP, record_exp_ty->exp_->UnEx(), new tree::ConstExp(field_offset));
  auto mem_exp = new tree::MemExp(bin_op_exp);
  exp_res = new tr::ExExp(mem_exp);

  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  /**
   *      MEM
   *       |
   *    ---+---
   *    |     |
   *   MEM   -*-
   *    |    | |
   *    e    i const(W)
  */
  tr::Exp* exp_res = nullptr;
  type::Ty* ty_res = nullptr;
  auto array_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  auto subscript_exp_ty = subscript_->Translate(venv, tenv, level, label, errormsg);
  ty_res = (static_cast<type::ArrayTy*>(array_exp_ty->ty_))->ty_->ActualTy();
  auto mem_exp = new tree::MemExp(
    new tree::BinopExp(tree::BinOp::PLUS_OP,
      array_exp_ty->exp_->UnEx(),
      new tree::BinopExp(tree::BinOp::MUL_OP,
        subscript_exp_ty->exp_->UnEx(),
        new tree::ConstExp(reg_manager->WordSize())
      )
    )
  );
  exp_res = new tr::ExExp(mem_exp);

  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(0)
    ),
    type::NilTy::Instance()
  );
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(val_)
    ),
    type::IntTy::Instance()
  );
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto frag_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(frag_label, str_));
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::NameExp(frag_label)),
    type::StringTy::Instance()
  );
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  // when call a function, we should add this function declare's level's static link as the first param
  /* TODO: Put your lab5 code here */
  tr::Exp* exp_res = nullptr;
  type::Ty* ty_res = nullptr;
  env::EnvEntry* func_entry = venv->Look(func_);
  auto func_label = (static_cast<env::FunEntry*>(func_entry))->label_;
  auto func_level = (static_cast<env::FunEntry*>(func_entry))->level_;
  ty_res = (static_cast<env::FunEntry*>(func_entry))->result_;
  auto call_exp_args_ = new tree::ExpList();
  // traverse the args list
  auto args_list = args_->GetList();
  for(auto &arg : args_list){
    auto arg_exp_ty = arg->Translate(venv, tenv, level, label, errormsg);
    call_exp_args_->Append(arg_exp_ty->exp_->UnEx());
  }
  //! callee maybe self defined function or sysyem provide function, need to handle with
  if(func_label != nullptr){
    auto call_exp_fun_ = new tree::NameExp(func_label);
    // push static link as the first param
    //! Attention: We should push func's parent fp here, not func's fp!!
    call_exp_args_->Insert(tr::FindFP(func_level->parent_, level));
    exp_res = new tr::ExExp(new tree::CallExp(call_exp_fun_, call_exp_args_));
  }
  else{
    exp_res = new tr::ExExp(frame::ExternalCall(func_->Name(), call_exp_args_));
  }

  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto left_exp_ty = left_->Translate(venv, tenv, level, label, errormsg);
  auto right_exp_ty = right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp* exp_res = nullptr;
  switch (oper_)
  {
  // 1. arithmetic operation
  case absyn::Oper::PLUS_OP:
  case absyn::Oper::MINUS_OP:
  case absyn::Oper::TIMES_OP:
  case absyn::Oper::DIVIDE_OP:
  {
    exp_res = new tr::ExExp(
      new tree::BinopExp(static_cast<tree::BinOp>(oper_ - 2), left_exp_ty->exp_->UnEx(), right_exp_ty->exp_->UnEx())
    );
    break;
  }
  // 2. relation operation
  //! attention: this place don't consider the string compare situation, maybe fix it later
  case absyn::Oper::EQ_OP:
  case absyn::Oper::NEQ_OP:
  case absyn::Oper::LT_OP:
  case absyn::Oper::LE_OP:
  case absyn::Oper::GT_OP:
  case absyn::Oper::GE_OP:
  { 
    if(oper_ == absyn::Oper::EQ_OP || oper_ == absyn::Oper::NEQ_OP){
      if(left_exp_ty->ty_->IsSameType(type::StringTy::Instance())){
        auto cjump_stm = new tree::CjumpStm(
          static_cast<tree::RelOp>(oper_ - 6),
          frame::ExternalCall(
            "string_equal",
            new tree::ExpList{
              left_exp_ty->exp_->UnEx(),
              right_exp_ty->exp_->UnEx()
            }
          ),
          new tree::ConstExp(1),
          nullptr,
          nullptr
        );
        std::list<temp::Label **> trues_patch_list;
        std::list<temp::Label **> falses_patch_list;
        trues_patch_list.clear();
        trues_patch_list.push_back(&cjump_stm->true_label_);
        falses_patch_list.clear();
        falses_patch_list.push_back(&cjump_stm->false_label_);
        tr::PatchList trues = tr::PatchList(trues_patch_list);
        tr::PatchList falses = tr::PatchList(falses_patch_list);

        exp_res = new tr::CxExp(trues, falses, cjump_stm);
        break;
      }
    }
    tree::RelOp rel_op;
    if(oper_ == absyn::Oper::GT_OP){
      rel_op = tree::RelOp::GT_OP;
    }
    else if(oper_ == absyn::Oper::LE_OP){
      rel_op = tree::RelOp::LE_OP;
    }
    else{
      rel_op = static_cast<tree::RelOp>(oper_ - 6);
    }
    auto cjump_stm = new tree::CjumpStm(
      rel_op,
      left_exp_ty->exp_->UnEx(),
      right_exp_ty->exp_->UnEx(),
      nullptr,
      nullptr
    );
    std::list<temp::Label **> trues_patch_list;
    std::list<temp::Label **> falses_patch_list;
    trues_patch_list.clear();
    trues_patch_list.push_back(&cjump_stm->true_label_);
    falses_patch_list.clear();
    falses_patch_list.push_back(&cjump_stm->false_label_);
    tr::PatchList trues = tr::PatchList(trues_patch_list);
    tr::PatchList falses = tr::PatchList(falses_patch_list);
    
    exp_res = new tr::CxExp(trues, falses, cjump_stm);
    break;
  }
  // 3. OR_OP, AND_OP (short-cut be considered)
  case absyn::Oper::AND_OP:
  {
    temp::Label* first_true_label = temp::LabelFactory::NewLabel();
    temp::Label* second_true_label = temp::LabelFactory::NewLabel();
    temp::Label* false_label = temp::LabelFactory::NewLabel();
    temp::Label* joint_label = temp::LabelFactory::NewLabel();
    temp::Temp* result_reg = temp::TempFactory::NewTemp(false);
    auto left_cx = left_exp_ty->exp_->UnCx(errormsg);
    auto right_cx = right_exp_ty->exp_->UnCx(errormsg);
    left_cx.trues_.DoPatch(first_true_label);
    left_cx.falses_.DoPatch(false_label);
    right_cx.trues_.DoPatch(second_true_label);
    right_cx.falses_.DoPatch(false_label);
    exp_res = new tr::ExExp(
      new tree::EseqExp(
        new tree::SeqStm(
          left_cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(first_true_label),
            new tree::SeqStm(
              right_cx.stm_,
              new tree::SeqStm(
                new tree::SeqStm(
                  new tree::LabelStm(second_true_label),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result_reg),
                      new tree::ConstExp(1)
                    ),
                    new tree::JumpStm(
                      new tree::NameExp(joint_label),
                      new std::vector<temp::Label *>{joint_label}
                    )
                  )
                ),
                new tree::SeqStm(
                  new tree::LabelStm(false_label),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result_reg),
                      new tree::ConstExp(0)
                    ),
                    new tree::LabelStm(joint_label)
                  )
                )
              )
            )
          )
        ),
        new tree::TempExp(result_reg)
      )
    );
    break;
  }
  case absyn::Oper::OR_OP:
  { 
    temp::Label* first_false_label = temp::LabelFactory::NewLabel();
    temp::Label* second_false_label = temp::LabelFactory::NewLabel();
    temp::Label* true_label = temp::LabelFactory::NewLabel();
    temp::Label* joint_label = temp::LabelFactory::NewLabel();
    temp::Temp* result_reg = temp::TempFactory::NewTemp(false);
    auto left_cx = left_exp_ty->exp_->UnCx(errormsg);
    auto right_cx = right_exp_ty->exp_->UnCx(errormsg);
    left_cx.trues_.DoPatch(true_label);
    left_cx.falses_.DoPatch(first_false_label);
    right_cx.trues_.DoPatch(true_label);
    right_cx.falses_.DoPatch(second_false_label);
    exp_res = new tr::ExExp(
      new tree::EseqExp(
        new tree::SeqStm(
          left_cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(first_false_label),
            new tree::SeqStm(
              right_cx.stm_,
              new tree::SeqStm(
                new tree::SeqStm(
                  new tree::LabelStm(second_false_label),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result_reg),
                      new tree::ConstExp(0)
                    ),
                    new tree::JumpStm(
                      new tree::NameExp(joint_label),
                      new std::vector<temp::Label *>{joint_label}
                    )
                  )
                ),
                new tree::SeqStm(
                  new tree::LabelStm(true_label),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result_reg),
                      new tree::ConstExp(1)
                    ),
                    new tree::LabelStm(joint_label)
                  )
                )
              )
            )
          )
        ),
        new tree::TempExp(result_reg)
      )
    );
    break;
  }
  default:
    break;
  }

  return new tr::ExpAndTy(exp_res, left_exp_ty->ty_->ActualTy());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty_res = tenv->Look(typ_)->ActualTy();
  /**
   * GC get the record's type's descriptor
  */
  if(typeid(*ty_res) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  auto record_ty = static_cast<type::RecordTy*>(ty_res);
  auto exp_field_list = fields_->GetList();
  auto result_reg = temp::TempFactory::NewTemp(true);
  // const auto ALLOC_SIZE = exp_field_list.size() * reg_manager->WordSize();
  /**
   * change alloc_record's param from a int to a pointer to descriptor
  */
  tree::Stm* alloc_move_stm = new tree::MoveStm(
    new tree::TempExp(result_reg),
    frame::ExternalCall(
      "alloc_record",
      new tree::ExpList{
        // new tree::ConstExp(ALLOC_SIZE)
        new tree::NameExp(record_ty->label_)
      }
    )
  );
  auto exp_field_list_it = exp_field_list.begin();
  tree::Stm* prev_node = alloc_move_stm;
  for(int i = 0;exp_field_list_it!=exp_field_list.end();++exp_field_list_it, ++i){
    auto exp_ty = (*exp_field_list_it)->exp_->Translate(venv, tenv, level, label, errormsg);
    prev_node = new tree::SeqStm(
      prev_node, 
      new tree::MoveStm(
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            new tree::TempExp(result_reg),
            new tree::ConstExp(i * reg_manager->WordSize())
          )
        ),
        exp_ty->exp_->UnEx()
      )
    );
  }
  tr::Exp* exp_res = new tr::ExExp(
    new tree::EseqExp(
      prev_node,
      new tree::TempExp(result_reg)
    )
  );

  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_list = seq_->GetList();
  auto exp_list_it = exp_list.begin();
  if(exp_list.size() == 0){
    // if there is no exp in this explist
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(exp_list.size() == 1){
    // if there is only one exp in this explist, then no need to use ESEQ, just return this Exp
    return (*exp_list_it)->Translate(venv, tenv, level, label, errormsg);
  }
  // if there are multiple exps in explist, we will use ESEQ as return Exp
  tree::Stm* prev_node = nullptr;
  for(;exp_list_it != exp_list.end();++exp_list_it){
    auto exp_ty = (*exp_list_it)->Translate(venv, tenv, level, label, errormsg);
    if(prev_node == nullptr){
      prev_node = exp_ty->exp_->UnNx();
      continue;
    }
    if(exp_list_it == std::prev(exp_list.end())){
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::EseqExp(
            prev_node,
            exp_ty->exp_->UnEx()
          )
        ),
        exp_ty->ty_->ActualTy()
      );
    }
    // other situation just use SEQ to put new stm in
    prev_node = new tree::SeqStm(
      prev_node,
      exp_ty->exp_->UnNx()
    );
  }

  // fake return
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  // no need to worry about the situation that assign a record or an array to a var
  // because in tiger, array and record are in heap, var is their pointer, so just assign new record/array's pointer to var
  auto exp_exp_ty = exp_->Translate(venv, tenv, level, label, errormsg);

  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::MoveStm(
        var_exp_ty->exp_->UnEx(),
        exp_exp_ty->exp_->UnEx()
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //! Is there any need to treat it specially?
  tr::Exp* exp_res = nullptr;
  type::Ty* ty_res = nullptr;
  // treat test_ as Cx, then_, elsee_ as Ex
  auto test_exp_ty = test_->Translate(venv, tenv, level, label, errormsg);
  auto then_exp_ty = then_->Translate(venv, tenv, level, label, errormsg);
  // allocate a register r, after label true, move then_ to r
  auto true_label = temp::LabelFactory::NewLabel();
  // after label false, move elsee_ to r
  auto false_label = temp::LabelFactory::NewLabel();

  auto test_cx = test_exp_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(true_label);
  test_cx.falses_.DoPatch(false_label);
  if(elsee_ != nullptr){
    // allocate a register r
    auto result_reg = temp::TempFactory::NewTemp(false);
    // finally jump to joint label
    auto joint_label = temp::LabelFactory::NewLabel();
    auto else_exp_ty = elsee_->Translate(venv, tenv, level, label, errormsg);
    auto eseq_exp = new tree::EseqExp(
      new tree::SeqStm(
        test_cx.stm_,
        new tree::SeqStm(
          new tree::LabelStm(true_label),
          new tree::SeqStm(
            new tree::SeqStm(
              new tree::MoveStm(
                new tree::TempExp(result_reg),
                then_exp_ty->exp_->UnEx()
              ),
              new tree::JumpStm(
                new tree::NameExp(joint_label),
                new std::vector<temp::Label *>{joint_label}
              )
            ),
            new tree::SeqStm(
              new tree::LabelStm(false_label),
              new tree::SeqStm(
                new tree::MoveStm(
                  new tree::TempExp(result_reg),
                  else_exp_ty->exp_->UnEx()
                ),
                new tree::LabelStm(joint_label)
              )
            )
          )
        )
      ),
      new tree::TempExp(result_reg)
    );
    exp_res = new tr::ExExp(eseq_exp);
    ty_res = then_exp_ty->ty_->ActualTy();
  }
  else{
    // If elsee_ is nullptr this means that then_ don't have value. then_exp_ty->ty_ = type::VoidTy::Instance()
    auto seq_stm = new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(true_label),
        new tree::SeqStm(
          then_exp_ty->exp_->UnNx(),
          // this situation don't need joint label anymore, false label is just enough
          new tree::LabelStm(false_label)
        )
      )
    );
    exp_res = new tr::NxExp(seq_stm);
    ty_res = then_exp_ty->ty_->ActualTy();
  }
  
  return new tr::ExpAndTy(exp_res, ty_res);
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp_res = nullptr;
  auto test_exp_ty = test_->Translate(venv, tenv, level, label, errormsg);
  auto test_cx = test_exp_ty->exp_->UnCx(errormsg);
  auto test_label = temp::LabelFactory::NewLabel();
  auto body_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto body_exp_ty = body_->Translate(venv, tenv, level, done_label, errormsg);
  test_cx.trues_.DoPatch(body_label);
  test_cx.falses_.DoPatch(done_label);
  exp_res = new tr::NxExp(
    new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
        test_cx.stm_,
        new tree::SeqStm(
          new tree::LabelStm(body_label),
          new tree::SeqStm(
            body_exp_ty->exp_->UnNx(),
            new tree::SeqStm(
              new tree::JumpStm(
                new tree::NameExp(test_label),
                new std::vector<temp::Label *>{test_label}
              ),
              new tree::LabelStm(done_label)
            )
          )
        )
      )
    )
  );
  return new tr::ExpAndTy(exp_res, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  auto body_label = temp::LabelFactory::NewLabel();
  auto loop_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto lo_exp_ty = lo_->Translate(venv, tenv, level, label, errormsg);
  auto hi_exp_ty = hi_->Translate(venv, tenv, level, label, errormsg);
  auto it_var_access = tr::Access::AllocLocal(level, escape_, false);
  venv->Enter(var_, new env::VarEntry(it_var_access, type::IntTy::Instance(), true));
  auto body_exp_ty = body_->Translate(venv, tenv, level, done_label, errormsg);
  //! Attention: don't think the pre check body and in the loop body is the same!
  //! You can't translate only one body and use it for two position, you should
  //! translate two body for pre check and loop body separately!!!!
  //! probably not correct//
  auto body_exp_ty_2 = body_->Translate(venv, tenv, level, done_label, errormsg);
  //! probably not correct//
  /**
   * let var i := lo
   *     var limit := hi
   * in
   * if i > limit goto done
   * body
   * if i == limit goto done
   * Loop: i := i + 1
   *       body
   *       if i < limit goto Loop
   * done
  */
  auto it_access_exp = it_var_access->access_->ToExp(tr::FindFP(level, level));
  auto init_it_stm = new tree::MoveStm(
    it_access_exp,
    lo_exp_ty->exp_->UnEx()
  );
  auto limit_reg = temp::TempFactory::NewTemp(false);
  auto init_limit_stm = new tree::MoveStm(
    new tree::TempExp(limit_reg),
    hi_exp_ty->exp_->UnEx()
  );
  auto pre_try_stm = new tree::CjumpStm(
    tree::RelOp::GT_OP,
    it_access_exp,
    new tree::TempExp(limit_reg),
    done_label,
    body_label
  );
  auto pre_check_stm = new tree::CjumpStm(
    tree::RelOp::EQ_OP,
    it_access_exp,
    new tree::TempExp(limit_reg),
    done_label,
    loop_label
  );
  auto loop_body_stm = new tree::SeqStm(
    new tree::MoveStm(
      it_access_exp,
      new tree::BinopExp(
        tree::BinOp::PLUS_OP,
        it_access_exp,
        new tree::ConstExp(1)
      )
    ),
    new tree::SeqStm(
      body_exp_ty->exp_->UnNx(),
      new tree::SeqStm(
        new tree::CjumpStm(
          tree::RelOp::LT_OP,
          it_access_exp,
          new tree::TempExp(limit_reg),
          loop_label,
          done_label
        ),
        new tree::LabelStm(done_label)
      )
    )
  );

  auto stm_res = new tree::SeqStm(
    init_it_stm,
    new tree::SeqStm(
      init_limit_stm,
      new tree::SeqStm(
        pre_try_stm,
        new tree::SeqStm(
          new tree::LabelStm(body_label),
          new tree::SeqStm(
            //! can't be the same as former
            body_exp_ty_2->exp_->UnNx(),
            new tree::SeqStm(
              pre_check_stm,
              new tree::SeqStm(
                new tree::LabelStm(loop_label),
                loop_body_stm
              )
            )
          )
        )
      )
    )
  );

  venv->EndScope();
  return new tr::ExpAndTy(
    new tr::NxExp(stm_res),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp_res = new tr::NxExp(
    new tree::JumpStm(
      new tree::NameExp(label),
      new std::vector<temp::Label *>{label}
    )
  );
  return new tr::ExpAndTy(exp_res, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  auto decs_list = decs_->GetList();
  tree::Stm* prev_node = nullptr;
  for(auto &dec : decs_list){
    auto dec_exp = dec->Translate(venv, tenv, level, label, errormsg);
    if(prev_node == nullptr){
      prev_node = dec_exp->UnNx();
      continue;
    }
    prev_node = new tree::SeqStm(
      prev_node,
      dec_exp->UnNx()
    );
  }
  tr::ExpAndTy* body_exp_ty;
  if(body_ == nullptr){
    body_exp_ty = new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
  }
  else{
    body_exp_ty = body_->Translate(venv, tenv, level, label, errormsg);
  }

  if(prev_node == nullptr){
    tenv->EndScope();
    venv->EndScope();
    return body_exp_ty;
  }
  tr::ExExp* exp_res = nullptr;
  exp_res = new tr::ExExp(
    new tree::EseqExp(
      prev_node,
      body_exp_ty->exp_->UnEx()
    )
  );
  tenv->EndScope();
  venv->EndScope();
  return new tr::ExpAndTy(
    exp_res,
    body_exp_ty->ty_->ActualTy()
  );
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty_res = tenv->Look(typ_)->ActualTy();
  auto size_exp_ty = size_->Translate(venv, tenv, level, label, errormsg);
  auto init_exp_ty = init_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(
    new tr::ExExp(
      frame::ExternalCall(
        "init_array",
        new tree::ExpList{
          size_exp_ty->exp_->UnEx(),
          init_exp_ty->exp_->UnEx()
        }
      )
    ),
    ty_res
  );
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(0)
    ),
    type::VoidTy::Instance()
  );
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto fun_decs = functions_->GetList();
  auto fun_dec_it = fun_decs.begin();
  for(;fun_dec_it != fun_decs.end();++fun_dec_it){
    auto formals = (*fun_dec_it)->params_->GetList();
    auto formal_ty_list = (*fun_dec_it)->params_->MakeFormalTyList(tenv, errormsg);
    type::Ty* result_ty = type::VoidTy::Instance();
    if((*fun_dec_it)->result_ != nullptr){
      result_ty = tenv->Look((*fun_dec_it)->result_)->ActualTy();
    }
    //! Use func name as label name? Will this cause some unexpected problem?
    auto fun_label = temp::LabelFactory::NamedLabel((*fun_dec_it)->name_->Name());
    auto formals_escape_list = new std::list<bool>();
    formals_escape_list->clear();
    for(auto &formal : formals){
      formals_escape_list->push_back(formal->escape_);
    }
    auto formals_is_pointer_list = new std::list<bool>();
    formals_is_pointer_list->clear();
    for(auto &formal_ty : formal_ty_list->GetList()){
      formals_is_pointer_list->push_back(type::isPointer(formal_ty));
    }
    // create a new level for this function
    auto fun_level =  new tr::Level(level, fun_label, formals_escape_list, formals_is_pointer_list);
    auto fun_entry = new env::FunEntry(fun_level, fun_label, formal_ty_list, result_ty);
    venv->Enter((*fun_dec_it)->name_, fun_entry);
  }

  fun_dec_it = fun_decs.begin();
  for(;fun_dec_it != fun_decs.end();++fun_dec_it){
    venv->BeginScope();
    auto fun_entry = static_cast<env::FunEntry *>(venv->Look((*fun_dec_it)->name_));
    auto frame_formal_access_list = fun_entry->level_->frame_->Formals();
    auto formal_list = fun_entry->formals_->GetList();
    auto param_list = (*fun_dec_it)->params_->GetList();
    //!Attention: the first of access list is static link!!
    auto access_it = frame_formal_access_list->begin();
    ++access_it;
    auto formal_it = formal_list.begin();
    auto param_it = param_list.begin();
    for(;param_it != param_list.end();++access_it, ++formal_it, ++param_it){
      //venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it, false));
      auto param_access = new tr::Access(fun_entry->level_, (*access_it));
      env::EnvEntry* param_entry = new env::VarEntry(param_access, *formal_it, false);
      venv->Enter((*param_it)->name_, param_entry);
    }
    auto body_exp_ty = (*fun_dec_it)->body_->Translate(venv, tenv, fun_entry->level_, label, errormsg);
    // we should do step7 in func dec(store func body's result into RV)
    tree::Stm* move_body_RV = new tree::MoveStm(
      new tree::TempExp(reg_manager->ReturnValue()),
      body_exp_ty->exp_->UnEx()
    );
    // complete view shift and create a fragment push into global fragment list
    frags->PushBack(frame::ProcEntryExit1(fun_entry->level_->frame_, move_body_RV));
    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto typ_ty = tenv->Look(typ_);
  bool is_pointer = type::isPointer(typ_ty);
  // call 'AllocLocal' to create a new local variable
  auto var_access = tr::Access::AllocLocal(level, escape_, is_pointer);
  auto init_exp_ty = init_->Translate(venv, tenv, level, label, errormsg);
  venv->Enter(var_, new env::VarEntry(var_access, init_exp_ty->ty_->ActualTy(), false));
  auto mem_exp = var_access->access_->ToExp(tr::FindFP(level, level));
  return new tr::NxExp(
    new tree::MoveStm(
      mem_exp,
      init_exp_ty->exp_->UnEx()
    )
  );
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto type_dec_list = types_->GetList();
  auto type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end();++type_dec_it){
    tenv->Enter((*type_dec_it)->name_, new type::NameTy((*type_dec_it)->name_, nullptr));
  }
  type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end();++type_dec_it){
    type::NameTy* name_ty = static_cast<type::NameTy *>(tenv->Look((*type_dec_it)->name_));
    name_ty->ty_ = (*type_dec_it)->ty_->Translate(tenv, errormsg);
  }
  /**
   * Create record's descriptor as a string fragment in .data section
   * Attention: we should create record's descriptor here
   * You can't create descriptor in RecordTy::Translate
   * because in RecordTy::Translate some record's field may be a pointer to another record
   * and its NameTy's ty_ is nullptr, so we can't judge whether this field is a pointer or not
   * just like "type node = { key: int, left: node, right: node }""
  */
  type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end();++type_dec_it){
    type::NameTy* name_ty = static_cast<type::NameTy *>(tenv->Look((*type_dec_it)->name_));
    auto actual_ty = name_ty->ty_->ActualTy();
    if(typeid(*actual_ty) == typeid(type::RecordTy)){
      auto record_ty = static_cast<type::RecordTy *>(actual_ty);
      if(record_ty->label_ == nullptr){
        auto field_list = record_ty->fields_->GetList();
        std::string descriptor = "";
        for(auto field : field_list){
          if(type::isPointer(field->ty_)){
            descriptor += "1";
          }
          else{
            descriptor += "0";
          }
        }
        record_ty->label_ = temp::LabelFactory::NewLabel();
        //!debug//
        // std::cout << name_ty->sym_->Name() << "'s descriptor created: " << descriptor << std::endl;
        // std::cout << "Field name list:" << std::endl;
        // for(auto field : field_list){
        //   std::cout << field->name_->Name() << std::endl;
        // } 
        //!debug//
        frags->PushBack(new frame::StringFrag(record_ty->label_, descriptor));
      }
    }
  }
  return new tr::ExExp(
    new tree::ConstExp(0) 
  );
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto name_ty = tenv->Look(name_);
  return new type::NameTy(name_, name_ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto type_fieldlist = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(type_fieldlist, nullptr);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* arr_ty = tenv->Look(array_);
  return new type::ArrayTy(arr_ty->ActualTy());
}

} // namespace absyn
