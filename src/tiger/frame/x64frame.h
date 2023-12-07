//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {

/**
 * 'InFrameAccess' and 'InRegAccess' are two derivative class of 'Access'
 * 'InRegAccess' will directly return TempExp(reg)
 * 'InFrameAccess' will first gain the FP(also a TempExp) and this var's offset in its frame
 * and then create a MemExp. If this var is not escaped, then return current fp directly.
 * Otherwise, we should use staticlink to find the true fp of this var.
*/
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr) const override;
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr) const override;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label* name, std::list<bool>* formals);
  frame::Access* AllocLocal(bool escape) override;
};

class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager();
  temp::TempList *Registers() override;
  temp::TempList *ArgRegs() override;
  temp::TempList *CallerSaves() override;
  temp::TempList *CalleeSaves() override;
  temp::TempList *ReturnSink() override;
  int WordSize() override;
  temp::Temp *FramePointer() override;
  temp::Temp *StackPointer() override;
  temp::Temp *ReturnValue() override;
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
