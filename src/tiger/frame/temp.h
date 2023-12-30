/*
* comment by: Bin Xu
* This module temp include 'Temp' and 'Label'. Temp is a data structure represents virtual registers. We will change these 
* temporary register to real register in "Register Allocation". Label represents the location of function's machine code begin.
*/

#ifndef TIGER_FRAME_TEMP_H_
#define TIGER_FRAME_TEMP_H_

#include "tiger/symbol/symbol.h"

#include <list>

namespace temp {

using Label = sym::Symbol;

class LabelFactory {
public:
  static Label *NewLabel();
  static Label *NamedLabel(std::string_view name);
  static std::string LabelString(Label *s);

private:
  int label_id_ = 0;
  static LabelFactory label_factory;
};

class Temp {
  friend class TempFactory;

public:
  bool is_pointer;
  [[nodiscard]] int Int() const;

private:
  int num_;
  explicit Temp(int num, bool isPointer) : num_(num), is_pointer(isPointer){}
};

class TempFactory {
public:
  static Temp *NewTemp(bool isPointer);

private:
  int temp_id_ = 100;
  static TempFactory temp_factory;
};

class Map {
public:
  void Enter(Temp *t, std::string *s);
  std::string *Look(Temp *t);
  void DumpMap(FILE *out);

  static Map *Empty();
  static Map *Name();
  static Map *LayerMap(Map *over, Map *under);

private:
  tab::Table<Temp, std::string> *tab_;
  Map *under_;

  Map() : tab_(new tab::Table<Temp, std::string>()), under_(nullptr) {}
  Map(tab::Table<Temp, std::string> *tab, Map *under)
      : tab_(tab), under_(under) {}
};

class TempList {
public:
  explicit TempList(Temp *t) : temp_list_({t}) {}
  TempList(std::initializer_list<Temp *> list) : temp_list_(list) {}
  TempList() = default;
  void Append(Temp *t) { temp_list_.push_back(t); }
  [[nodiscard]] Temp *NthTemp(int i) const;
  [[nodiscard]] const std::list<Temp *> &GetList() const { return temp_list_; }
  // some support function used for lab6
  static TempList* UnionTempList(TempList* s1, TempList* s2){
    auto res = new TempList();
    if(s1 != nullptr){
      auto s1_list = s1->GetList();
      for(auto temp : s1_list){
        res->Append(temp);
      }
    }
    if(s2 != nullptr){
      auto res_list = res->GetList();
      auto s2_list = s2->GetList();
      for(auto s2_temp : s2_list){
        bool is_exist = false;
        for(auto res_temp : res_list){
          if(res_temp == s2_temp){
            is_exist = true;
            break;
          }
        }
        if(!is_exist){
          res->Append(s2_temp);
        }
      }
    }

    return res;
  }

  static TempList* DiffTempList(TempList* s1, TempList* s2){
    auto res = new TempList();
    if(s1 == nullptr){
      return res;
    }
    auto s1_list = s1->GetList();
    if(s2 == nullptr){
      for(auto temp : s1_list){
        res->Append(temp);
      }
      return res;
    }
    auto s2_list = s2->GetList();
    for(auto s1_temp : s1_list){
      bool is_exist = false;
      for(auto s2_temp : s2_list){
        if(s1_temp == s2_temp){
          is_exist = true;
          break;
        }
      }
      if(!is_exist){
        res->Append(s1_temp);
      }
    }

    return res;
  }

  static TempList* RewriteTempList(TempList* s1, Temp* _old, Temp* _new){
    auto res = new TempList();
    if(s1 == nullptr){
      return res;
    }
    auto s1_list = s1->GetList();
    for(auto s1_temp : s1_list){
      if(s1_temp == _old){
        res->Append(_new);
      }
      else{
        res->Append(s1_temp);
      }
    }
    return res;
  }

private:
  std::list<Temp *> temp_list_;
};

} // namespace temp

#endif