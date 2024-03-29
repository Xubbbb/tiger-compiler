#include "tiger/semant/types.h"

namespace type {

NilTy NilTy::nilty_;
IntTy IntTy::intty_;
StringTy StringTy::stringty_;
VoidTy VoidTy::voidty_;

Ty *Ty::ActualTy() { return this; }

Ty *NameTy::ActualTy() {
  assert(ty_ != this);
  return ty_->ActualTy();
}

bool Ty::IsSameType(Ty *expected) {
  Ty *a = ActualTy();
  Ty *b = expected->ActualTy();

  if ((typeid(*a) == typeid(NilTy) && typeid(*b) == typeid(RecordTy)) ||
      (typeid(*a) == typeid(RecordTy) && typeid(*b) == typeid(NilTy)))
    return true;

  return a == b;
}

bool isPointer(Ty* ty){
  auto actual_ty = ty->ActualTy();
  return typeid(*actual_ty) == typeid(type::ArrayTy) || typeid(*actual_ty) == typeid(type::RecordTy) || typeid(*actual_ty) == typeid(type::NilTy);
}

} // namespace type
