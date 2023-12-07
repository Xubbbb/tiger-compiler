/**
 * comment by: Bin Xu
 * the implementation of frame.h is specific to different target machine.The interface is abstract.
 */

#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"


namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  virtual tree::Exp *ToExp(tree::Exp* framePtr) const = 0;
  virtual ~Access() = default;
};

class Frame {
public:
  /* TODO: Put your lab5 code here */
  temp::Label* name_;
  // denoting the locations where the formal parameters will be kept at run time as callee's view
  std::list<frame::Access *> *formals;
  // record if a new local var is allocated on the stack, what offset will it get
  int offset;

  Frame(){}
  /**
   * when a new function g is called, we should call NewFrame(g, ...) to create a new frame for this function
   * 1. NewFrame should handle with 'shift of view': 
   *    (1) The param is in a register or in a frame location
   *    (2) Instructions produced to implement 'view shift'
   * 
   *
   * @param name the label represents the function's machine code begin
   * @param formals whether this formal is escaped
   */
  Frame(temp::Label* name, std::list<bool>* formals)
    :name_(name),
    formals(nullptr)
  {
  }

  /**
   * If true, Returns an InFrameAccess with an offset from the frame pointer
   * (Frame size may also be optimized by noticing when two frame-resident variables could be allocated to the same slot)
   * If false, Returns a register InRegAccess(t481)
   * (Register allocator will use as few registers as possible to represent the temporary)
   * 
   * @param escape whether this formal is escaped
  */
  virtual frame::Access* AllocLocal(bool escape) = 0;

  /**
   * return the formals list of this frame
  */
  std::list<frame::Access *>* Formals() const {
    return this->formals;
  }
  std::string GetLabel() const {
    return name_->Name();
  }
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */

/**
 * ExternalCall helps you call runtime.c providing function
*/
tree::Exp* ExternalCall(std::string s, tree::ExpList* args);

/**
 * ProcEntryExit1:
 * This function should complete step4, 5, 8 in func dec(view shift)
 * 4: receive input params
 * 5: save callee saved register
 * 8: restore callee saved register
*/
frame::ProcFrag* ProcEntryExit1(frame::Frame* frame_, tree::Stm* stm_);

assem::InstrList* ProcEntryExit2(assem::InstrList *);

assem::Proc* ProcEntryExit3(frame::Frame* frame_, assem::InstrList* il);

} // namespace frame

#endif