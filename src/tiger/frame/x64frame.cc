#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
};
/* TODO: Put your lab5 code here */

X64RegManager::X64RegManager(){

}

temp::TempList *X64RegManager::Registers() {
  return nullptr;
}

temp::TempList *X64RegManager::ArgRegs() {
  return nullptr;
}

temp::TempList *X64RegManager::CallerSaves() {
  return nullptr;
}

temp::TempList *X64RegManager::CalleeSaves() {
  return nullptr;
}

temp::TempList *X64RegManager::ReturnSink() {
  return nullptr;
}

int X64RegManager::WordSize() {
  return 0;
}

temp::Temp *X64RegManager::FramePointer() {
  return nullptr;
}

temp::Temp *X64RegManager::StackPointer() {
  return nullptr;
}

temp::Temp *X64RegManager::ReturnValue() {
  return nullptr;
}

} // namespace frame