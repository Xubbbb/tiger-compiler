#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int max_args_left = stm1->MaxArgs();
  int max_args_right =  stm2->MaxArgs();

  return max_args_left >= max_args_right ? max_args_left : max_args_right;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  //先执行左边的代码再执行右边的代码
  Table * left_stm_result = stm1->Interp(t);
  Table * stm_result = stm2->Interp(left_stm_result);

  return stm_result;
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable * right_exp_result = exp->Interp(t);
  int right_exp_value = right_exp_result->i;
  Table * right_exp_table = right_exp_result->t;
  // 这里需要注意由于Update并不是一个自我更新的函数，它是将更新过的Table的头节点返回，而不是将调用Update的指针指向新的Table的头节点
  Table * result_table = right_exp_table->Update(id, right_exp_value);

  return result_table;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int exps_args = exps->NumExps();
  int exps_max_args = exps->MaxArgs();

  return exps_args >= exps_max_args ? exps_args : exps_max_args;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return exps->Interp(t)->t;
}

int A::IdExp::MaxArgs() const {
  //Id表达式无参数
  return 0;
}

IntAndTable * A::IdExp::Interp(Table * t) const {
  //Id表达式返回当前变量的值，对于存储值的链表不做修改
  IntAndTable * result = new IntAndTable(t->Lookup(id), t);

  return result;
}

int A::NumExp::MaxArgs() const {
  return 0;
}

IntAndTable * A::NumExp::Interp(Table *t) const {
  IntAndTable * result = new IntAndTable(num, t);

  return result;
}

int A::OpExp::MaxArgs() const {
  int left_exp_args = left->MaxArgs();
  int right_exp_args = right->MaxArgs();

  return left_exp_args >= right_exp_args ? left_exp_args : right_exp_args;
}

IntAndTable * A::OpExp::Interp(Table *t) const {
  IntAndTable * left_exp_result = left->Interp(t);
  IntAndTable * right_exp_result = right->Interp(left_exp_result->t);
  int result_value = 0;
  switch (oper)
  {
  case PLUS:
    result_value = left_exp_result->i + right_exp_result->i;
    break;
  case MINUS:
    result_value = left_exp_result->i - right_exp_result->i;
    break;
  case TIMES:
    result_value = left_exp_result->i * right_exp_result->i;
    break;
  default:
    result_value = left_exp_result->i / right_exp_result->i;
    break;
  }
  IntAndTable * result = new IntAndTable(result_value, right_exp_result->t);

  return result;
}

int A::EseqExp::MaxArgs() const {
  int stm_args = stm->MaxArgs();
  int exp_args = exp->MaxArgs();

  return stm_args >= exp_args ? stm_args : exp_args;
}

IntAndTable * A::EseqExp::Interp(Table *t) const {
  //先执行Stm再执行Exp, 并返回Exp的结果
  Table * stm_result = stm->Interp(t);

  return exp->Interp(stm_result);
}

int A::PairExpList::MaxArgs() const {
  int left_exp_args = exp->MaxArgs();
  int right_exps_args = tail->MaxArgs();

  return left_exp_args >= right_exps_args ? left_exp_args : right_exps_args;
}

int A::PairExpList::NumExps() const {
  return tail->NumExps() + 1;
}

IntAndTable * A::PairExpList::Interp(Table *t) const {
  IntAndTable * left_exp_result = exp->Interp(t);
  std::cout << left_exp_result->i << " ";
  
  return tail->Interp(left_exp_result->t);
}

int A::LastExpList::MaxArgs() const {
  return exp->MaxArgs();
}

int A::LastExpList::NumExps() const {
  return 1;
}

IntAndTable * A::LastExpList::Interp(Table *t) const {
  IntAndTable * exp_result = exp->Interp(t);
  //这里注意每一个print()打印完以后都应该换行, 故LastExp中输出的应该是一个换行符
  std::cout << exp_result->i << std::endl;

  return exp_result;
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  //注意Update并不是对自身对象的更新, 而是将更新过后的对象返回
  return new Table(key, val, this);
}
}  // namespace A
