#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <iostream>

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry* entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)){
    return (static_cast<env::VarEntry*>(entry))->ty_->ActualTy();
  }
  else{
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* record_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if(typeid(*record_ty) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "not a record type");
    return type::VoidTy::Instance();
  }

  const std::list<type::Field *> record_field_list = (static_cast<type::RecordTy*>(record_ty))->fields_->GetList();
  for(type::Field* field:record_field_list){
    if(field->name_ == sym_){
      return field->ty_->ActualTy();
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arr_var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(typeid(*arr_var_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  type::Ty* subscript_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(typeid(*subscript_ty) != typeid(type::IntTy)){
    errormsg->Error(pos_, "subscript must be a int");
  }

  return (static_cast<type::ArrayTy*>(arr_var_ty))->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  return var_ty;
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry* func_entry = venv->Look(func_);
  // if(func_entry && typeid(*func_entry) == typeid(env::FunEntry)){
  //   return static_cast<env::FunEntry*>(func_entry)->result_->ActualTy();
  // }
  if(func_entry == nullptr){
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::VoidTy::Instance();
  }
  if(typeid(*func_entry) != typeid(env::FunEntry)){
    errormsg->Error(pos_, "%s is not a function", func_->Name().data());
    return type::VoidTy::Instance();
  }
  std::list<type::Ty*> func_formal_ty_list = (static_cast<env::FunEntry*>(func_entry))->formals_->GetList();
  std::list<absyn::Exp*> args_list = args_->GetList();

  if(args_list.size() < func_formal_ty_list.size()){
    errormsg->Error(pos_, "para type mismatch");
    return (static_cast<env::FunEntry*>(func_entry))->result_;
  }
  else if(args_list.size() > func_formal_ty_list.size()){
    errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
    return (static_cast<env::FunEntry*>(func_entry))->result_;
  }

  std::list<type::Ty*>::iterator formal_it = func_formal_ty_list.begin();
  std::list<absyn::Exp*>::iterator arg_it = args_list.begin();
  
  for(;formal_it != func_formal_ty_list.end();++formal_it, ++arg_it){
    type::Ty* arg_ty = (*arg_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!(*formal_it)->IsSameType(arg_ty)){
      errormsg->Error(pos_, "para type mismatch");
    }
  }

  return (static_cast<env::FunEntry*>(func_entry))->result_;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  //AcutalTy函数仅仅对于NameTy去找到这个类型别名的真正类型
  //获取OP表达式二元各自的类型
  type::Ty* left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty* right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if(oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP || oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP){
    if(!left_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(left_->pos_, "integer required!");
    }
    if(!right_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(right_->pos_, "integer required!");
    }
    return type::IntTy::Instance();
  }
  else{
    if(!left_ty->IsSameType(right_ty)){
      errormsg->Error(pos_, "same type required");
      return type::IntTy::Instance();
    }
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* record_ty = tenv->Look(typ_);
  if(record_ty == nullptr){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::VoidTy::Instance();
  }
  
  if(typeid(*(record_ty->ActualTy())) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "%s is not a record", typ_->Name().data());
    return record_ty->ActualTy();
  }
  
  std::list<type::Field *> record_field_list = (static_cast<type::RecordTy *>(record_ty->ActualTy()))->fields_->GetList();
  std::list<absyn::EField *> exp_field_list = fields_->GetList();

  if(record_field_list.size() != exp_field_list.size()){
    errormsg->Error(pos_, "num of fields is wrong");
    return record_ty->ActualTy();
  }
  
  std::list<type::Field *>::iterator record_field_list_it = record_field_list.begin();
  std::list<absyn::EField *>::iterator exp_field_list_it = exp_field_list.begin();

  for(;record_field_list_it != record_field_list.end();++record_field_list_it, ++exp_field_list_it){
    if((*record_field_list_it)->name_ != (*exp_field_list_it)->name_){
      errormsg->Error(pos_, "field name is wrong");
      return record_ty->ActualTy();
    }
    type::Ty* efield_ty = (*exp_field_list_it)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
    type::Ty* field_ty = (*record_field_list_it)->ty_;
    if(!efield_ty->IsSameType(field_ty)){
      errormsg->Error(pos_, "field type wrong");
      return record_ty->ActualTy();
    }
  }

  return record_ty->ActualTy();
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<absyn::Exp *> exp_list = seq_->GetList();
  std::list<absyn::Exp *>::iterator exp_list_it = exp_list.begin();

  type::Ty* result_ty;

  for(;exp_list_it != exp_list.end();++exp_list_it){
    result_ty = (*exp_list_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  return result_ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  
  if(typeid(*var_) == typeid(absyn::SimpleVar)){
    env::EnvEntry* var_entry = venv->Look((static_cast<SimpleVar *>(var_))->sym_);
    if(typeid(*var_entry) == typeid(env::VarEntry)){
      // 需要检查此处这个赋值语句有可能是在循环体当中的，那么它不能修改哨兵的值
      if(static_cast<env::VarEntry *>(var_entry)->readonly_ == true){
        errormsg->Error(pos_, "loop variable can't be assigned");
      }
    }
    else{
      errormsg->Error(pos_, "assign lvalue is not a var");
    }
  }

  type::Ty* exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!var_ty->IsSameType(exp_ty)){
    errormsg->Error(pos_, "unmatched assign exp");
  }

  // 注意赋值表达式不产生值
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!test_ty->IsSameType(type::IntTy::Instance())){
    errormsg->Error(pos_, "the test of ifexp must be an int");
  }
  type::Ty* then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(elsee_ != nullptr){
    type::Ty* else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!then_ty->IsSameType(else_ty)){
      errormsg->Error(pos_, "then exp and else exp type mismatch");
    }
  }
  else{
    if(!then_ty->IsSameType(type::VoidTy::Instance())){
      errormsg->Error(pos_, "if-then exp's body must produce no value");
    }
  }
  return then_ty->ActualTy();
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!test_ty->IsSameType(type::IntTy::Instance())){
    errormsg->Error(pos_, "the test of while exp must be a int");
  }
  //进入循环体时需要将label count增加
  type::Ty* body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg)->ActualTy();
  if(typeid(*body_ty) != typeid(type::VoidTy)){
    errormsg->Error(pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  type::Ty* lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty* hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!lo_ty->IsSameType(type::IntTy::Instance()) || !hi_ty->IsSameType(type::IntTy::Instance())){
    errormsg->Error(pos_, "for exp's range type is not integer");
  }
  //注意循环的哨兵在body中是不可以修改的，故readonly为true
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  //进入循环体时需要将label count增加
  type::Ty* body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if(!body_ty->IsSameType(type::VoidTy::Instance())){
    errormsg->Error(pos_, "body must be a void exp");
  }
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(labelcount == 0){
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // 开启一个新的环境,压入代表scope开始的标记
  venv->BeginScope();
  tenv->BeginScope();
  // 遍历声明中的每一个语句进行type checking和修改venv和tenv

  // std::list<absyn::Dec *> dec_list = decs_->GetList();
  // std::list<absyn::Dec *>
  for(Dec *dec : decs_->GetList()){
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  type::Ty* result;
  if(!body_){
    result = type::VoidTy::Instance();
  }
  else{
    result = body_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  }
  tenv->EndScope();
  venv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arr_ty = tenv->Look(typ_)->ActualTy();
  if(arr_ty == nullptr){
    errormsg->Error(pos_, "%s is not defined", typ_->Name().data());
  }
  if(typeid(*arr_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "%s is not an array", typ_->Name().data());
  }

  type::Ty* arr_element_ty = (static_cast<type::ArrayTy *>(arr_ty))->ty_->ActualTy();
  type::Ty* size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty* init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if(!size_ty->IsSameType(type::IntTy::Instance())){
    errormsg->Error(pos_, "size must be a int");
  }
  if(!arr_element_ty->IsSameType(init_ty)){
    errormsg->Error(pos_, "type mismatch");
  }

  return arr_ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // 为了解决有递归函数的声明，函数声明需要分为两步
  // 首先first pass将每个函数的函数原型(func name, parameter type, return type)加入到venv中
  std::list<absyn::FunDec *> fun_decs = functions_->GetList();
  std::list<absyn::FunDec *>::iterator fun_dec_it = fun_decs.begin();
  for(;fun_dec_it != fun_decs.end();++fun_dec_it){
    // 检查是否有函数重名
    if(venv->Look((*fun_dec_it)->name_) != nullptr){
      errormsg->Error(pos_, "two functions have the same name");
      return;
    }
    type::TyList* formals = (*fun_dec_it)->params_->MakeFormalTyList(tenv, errormsg);
    type::Ty* result_ty = type::VoidTy::Instance();
    if((*fun_dec_it)->result_ != nullptr){
      result_ty = tenv->Look((*fun_dec_it)->result_)->ActualTy();
      if(result_ty == nullptr){
        errormsg->Error(pos_, "return type is not exist");
        return;
      }
    }
    venv->Enter((*fun_dec_it)->name_, new env::FunEntry(formals, result_ty));
  }
  // 然后利用这个新的环境去重新检查
  fun_dec_it = fun_decs.begin();
  for(;fun_dec_it != fun_decs.end();++fun_dec_it){
    venv->BeginScope();
    // 先将函数声明头中的变量名添加到新的环境中
    env::FunEntry* fun_entry = static_cast<env::FunEntry *>(venv->Look((*fun_dec_it)->name_));
    std::list<type::Ty *> formal_list = fun_entry->formals_->GetList();
    std::list<absyn::Field *> param_list = (*fun_dec_it)->params_->GetList();
    std::list<type::Ty *>::iterator formal_it = formal_list.begin();
    std::list<absyn::Field *>::iterator param_it = param_list.begin();
    for(;param_it != param_list.end();++formal_it, ++param_it){
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it, false));
    }
    type::Ty* body_ty = (*fun_dec_it)->body_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
    if(!body_ty->IsSameType(fun_entry->result_)){
      errormsg->Error(pos_, "procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(typ_ == nullptr){
    if(init_ty->IsSameType(type::NilTy::Instance())){
      errormsg->Error(pos_, "init should not be nil without type specified");
    }
    venv->Enter(var_, new env::VarEntry(init_ty));
  }
  else{
    type::Ty* typ_ty = tenv->Look(typ_);
    if(typ_ty == nullptr){
      errormsg->Error(pos_, "this type is not defined");
      venv->Enter(var_, new env::VarEntry(init_ty));
      return;
    }
    if(!typ_ty->IsSameType(init_ty)){
      errormsg->Error(pos_, "type mismatch");
    }
    venv->Enter(var_, new env::VarEntry(typ_ty));
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // 首先第一遍先遍历所有的type dec
  std::list<absyn::NameAndTy *> type_dec_list = types_->GetList();
  std::list<absyn::NameAndTy *>::iterator type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end();++type_dec_it){
    if(tenv->Look((*type_dec_it)->name_) != nullptr){
      errormsg->Error(pos_, "two types have the same name");
      return;
    }
    tenv->Enter((*type_dec_it)->name_, new type::NameTy((*type_dec_it)->name_, nullptr));
  }
  // 第二遍确定每个NameAndTy的type
  type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end(); ++type_dec_it){
    type::NameTy* name_ty = static_cast<type::NameTy *>(tenv->Look((*type_dec_it)->name_));
    name_ty->ty_ = (*type_dec_it)->ty_->SemAnalyze(tenv, errormsg);
  }
  // 检查是否会产生类型定义的循环
  bool is_endless_loop = false;
  type_dec_it = type_dec_list.begin();
  for(;type_dec_it != type_dec_list.end(); ++type_dec_it){
    type::NameTy* name_ty = static_cast<type::NameTy *>(tenv->Look((*type_dec_it)->name_));
    type::NameTy* name_ty_it = name_ty;
    // 一直往下找如果找到真的类型如果找到自己了就代表循环了
    while(true){
      type::Ty* next_ty = name_ty_it->ty_;
      if(typeid(*next_ty) != typeid(type::NameTy)){
       break; 
      }
      else{
        if(static_cast<type::NameTy *>(next_ty)->sym_ == name_ty->sym_){
          is_endless_loop = true;
          break;
        }
      }
      name_ty_it = static_cast<type::NameTy *>(next_ty);
    }
    if(is_endless_loop){
      errormsg->Error(pos_, "illegal type cycle");
      break;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* name_ty = tenv->Look(name_);
  if(name_ty == nullptr){
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, name_ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arr_ty = tenv->Look(array_);
  if(arr_ty == nullptr){
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(arr_ty->ActualTy());
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
