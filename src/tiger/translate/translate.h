#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

class PatchList {
public:
  void DoPatch(temp::Label *label) {
    for(auto &patch : patch_list_) *patch = label;
  }

  static PatchList JoinPatch(const PatchList &first, const PatchList &second) {
    PatchList ret(first.GetList());
    for(auto &patch : second.patch_list_) {
      ret.patch_list_.push_back(patch);
    }
    return ret;
  }

  explicit PatchList(std::list<temp::Label **> patch_list) : patch_list_(patch_list) {}
  PatchList() = default;

  [[nodiscard]] const std::list<temp::Label **> &GetList() const {
    return patch_list_;
  }

private:
  std::list<temp::Label **> patch_list_;
};

/**
 * In the phase of semantic analysis,
 * VarDec::Translate creates a “new location” tr::Access for each variable in level
 * (by calling tr::Access::AllocLocal(level, escape_))
 * Semant records this tr::Access in its VarEntry
 * Semant uses this access later when generating machine code
*/
class Access {
public:
  Level *level_;
  frame::Access *access_;

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}
  static Access *AllocLocal(Level *level, bool escape, bool is_pointer);
};

/**
 * In the phase of semantic analysis,
 * FunctionDec::Translate creates a new “nesting level” for each function by calling tr::Level::NewLevel(),
 * NewLevel will call frame::Frame::NewFrame() to create a new frame.
 * Semant keeps this 'level' in its FunEntry.
*/
class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  /* TODO: Put your lab5 code here */
  Level(tr::Level *parent, temp::Label *name, std::list<bool> *formals, std::list<bool> *is_pointer);
};

class ProgTr {
public:
  // TODO: Put your lab5 code here */
  ProgTr(std::unique_ptr<absyn::AbsynTree> absyn_tree, std::unique_ptr<err::ErrorMsg> error_msg)
    :absyn_tree_(std::move(absyn_tree)),
    errormsg_(std::move(error_msg)),
    main_level_(new tr::Level(nullptr, temp::LabelFactory::NamedLabel("tigermain"), nullptr, nullptr)),
    tenv_(new env::TEnv),
    venv_(new env::VEnv){
      FillBaseTEnv();
      FillBaseVEnv();
    }

  /**
   * Translate IR tree
   */
  void Translate();

  /**
   * Transfer the ownership of errormsg to outer scope
   * @return unique pointer to errormsg
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }


private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  // Fill base symbol for var env and type env
  void FillBaseVEnv();
  void FillBaseTEnv();
};

} // namespace tr

#endif
