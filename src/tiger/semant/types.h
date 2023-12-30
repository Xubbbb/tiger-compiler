#ifndef TIGER_SEMANT_TYPES_H_
#define TIGER_SEMANT_TYPES_H_

#include "tiger/symbol/symbol.h"
#include "tiger/frame/temp.h"
#include <list>

namespace type {

class TyList;
class Field;
class FieldList;

class Ty {
public:
  virtual Ty *ActualTy();
  virtual bool IsSameType(Ty *);

protected:
  Ty() = default;
};

class NilTy : public Ty {
public:
  static NilTy *Instance() { return &nilty_; }

private:
  static NilTy nilty_;
};

class IntTy : public Ty {
public:
  static IntTy *Instance() { return &intty_; }

private:
  static IntTy intty_;
};

class StringTy : public Ty {
public:
  static StringTy *Instance() { return &stringty_; }

private:
  static StringTy stringty_;
};

class VoidTy : public Ty {
public:
  static VoidTy *Instance() { return &voidty_; }

private:
  static VoidTy voidty_;
};

class RecordTy : public Ty {
public:
  FieldList *fields_;
  /**
   * When a record type was first declared in code
   * we will generate a fragment for its descriptor in .data just like
   * string and and give this fragment a label.
   * We should store this label in this record type.
   * When a record instance was created, we should
   * pass this fragment label(present descriptor's address)
   * to external function alloc_record
  */
  temp::Label*  label_;

  explicit RecordTy(FieldList *fields) : fields_(fields),label_(nullptr) {}
  explicit RecordTy(FieldList *fields, temp::Label* label) : fields_(fields), label_(label) {}
};

class ArrayTy : public Ty {
public:
  Ty *ty_;

  explicit ArrayTy(Ty *ty) : ty_(ty) {}
};

class NameTy : public Ty {
public:
  sym::Symbol *sym_;
  Ty *ty_;

  NameTy(sym::Symbol *sym, Ty *ty) : sym_(sym), ty_(ty) {}

  Ty *ActualTy() override;
};

class TyList {
public:
  TyList() = default;
  explicit TyList(Ty *ty) : ty_list_({ty}) {}
  TyList(std::initializer_list<Ty *> list) : ty_list_(list) {}

  const std::list<Ty *> &GetList() { return ty_list_; }
  void Append(Ty *ty) { ty_list_.push_back(ty); }

private:
  std::list<Ty *> ty_list_;
};

class Field {
public:
  sym::Symbol *name_;
  Ty *ty_;

  Field(sym::Symbol *name, Ty *ty) : name_(name), ty_(ty) {}
};

class FieldList {
public:
  FieldList() = default;
  explicit FieldList(Field *field) : field_list_({field}) {}

  std::list<Field *> &GetList() { return field_list_; }
  void Append(Field *field) { field_list_.push_back(field); }

private:
  std::list<Field *> field_list_;
};

bool isPointer(Ty* ty);

} // namespace type

#endif // TIGER_SEMANT_TYPES_H_
