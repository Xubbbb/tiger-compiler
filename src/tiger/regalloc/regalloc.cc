#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include <iostream>

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */


RegAllocator::RegAllocator(frame::Frame* frame, std::unique_ptr<cg::AssemInstr> assem_instr)
    :frame_(frame),
    assem_instr_(std::move(assem_instr)),
    color_(nullptr)
{
    color_ = new col::Color(assem_instr_->GetInstrList());
}
RegAllocator::~RegAllocator(){

}

void RegAllocator::RegAlloc(){
    while(true){
        auto color_res = color_->RAColor();
        if(color_res.spills != nullptr){
            RewriteProgram(color_res.spills);
        }
        else{
            coloring_ = color_res.coloring;
            break;
        }
    }
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult(){
    auto res = std::make_unique<ra::Result>();
    res->coloring_ = coloring_;
    
    //! We haven't delete those coallesced move instr,
    //! we choose to delete them here
    auto delete_move_list = new assem::InstrList();
    for(auto instr_it = assem_instr_->GetInstrList()->GetList().begin();instr_it != assem_instr_->GetInstrList()->GetList().end();++instr_it){
        if(typeid(*(*instr_it)) == typeid(assem::MoveInstr)){
            auto move_instr = static_cast<assem::MoveInstr*>(*instr_it);
            auto src_reg = coloring_->Look(move_instr->src_->NthTemp(0));
            auto dst_reg = coloring_->Look(move_instr->dst_->NthTemp(0));
            if(src_reg == dst_reg){
                delete_move_list->Append(*instr_it);
            }
        }
    }
    auto coalesced_list = delete_move_list->GetList();
    for(auto instr : coalesced_list){
        assem_instr_->GetInstrList()->Remove(instr);
    }

    res->il_ = assem_instr_->GetInstrList();
    return std::move(res);
}

void RegAllocator::RewriteProgram(live::INodeListPtr spilledNodes){
    auto spilled_list = spilledNodes->GetList();
    for(auto v : spilled_list){
        auto v_temp = v->NodeInfo();
        auto v_access = static_cast<frame::InFrameAccess*>(frame_->AllocLocal(true));
        //! Attention: This place could not use instr_list, must use assem_instr_->GetInstrList()->GetList()
        //! Initially, I think these two are the same because 'GetList()' will return a 'list&'
        //! But actually it seems not the same. Maybe I will work out the reason later.
        //auto instr_list = assem_instr_->GetInstrList()->GetList();
        for(auto instr_it = assem_instr_->GetInstrList()->GetList().begin();instr_it != assem_instr_->GetInstrList()->GetList().end();++instr_it){
            //! Attention: Be careful about iterator. This place should use 2 '*' to get the instance
            if(typeid(*(*instr_it)) == typeid(assem::OperInstr)){
                auto oper_instr = static_cast<assem::OperInstr*>(*instr_it);
                // If in use
                auto instr_use = oper_instr->Use();
                if(instr_use != nullptr){
                    auto use_list = instr_use->GetList();
                    bool is_exist = false;
                    for(auto use : use_list){
                        if(use == v_temp){
                            is_exist = true;
                            break;
                        }
                    }
                    if(is_exist){
                        auto replace_temp = temp::TempFactory::NewTemp();
                        oper_instr->src_ = temp::TempList::RewriteTempList(instr_use, v_temp, replace_temp);
                        // insert new instruction
                        std::string instr_assem = "movq (" + frame_->GetLabel() + "_framesize" + std::to_string(v_access->offset) + ")(`s0), `d0";
                        auto src_list = new temp::TempList();
                        auto dst_list = new temp::TempList();
                        src_list->Append(reg_manager->StackPointer());
                        dst_list->Append(replace_temp);
                        assem::Instr* new_instr = new assem::OperInstr(
                            instr_assem,
                            dst_list,
                            src_list,
                            nullptr
                        );
                        assem_instr_->GetInstrList()->Insert(instr_it, new_instr);
                    }
                }

                // If in def
                auto instr_def = oper_instr->Def();
                if(instr_def != nullptr){
                    auto def_list = instr_def->GetList();
                    bool is_exist = false;
                    for(auto def : def_list){
                        if(def == v_temp){
                            is_exist = true;
                            break;
                        }
                    }
                    if(is_exist){
                        auto replace_temp = temp::TempFactory::NewTemp();
                        oper_instr->dst_ = temp::TempList::RewriteTempList(instr_def, v_temp, replace_temp);
                        std::string instr_assem = "movq `s0, (" + frame_->GetLabel() + "_framesize" + std::to_string(v_access->offset) + ")(`s1)";
                        auto src_list = new temp::TempList();
                        src_list->Append(replace_temp);
                        src_list->Append(reg_manager->StackPointer());
                        assem::Instr* new_instr = new assem::OperInstr(
                            instr_assem,
                            nullptr,
                            src_list,
                            nullptr
                        );
                        assem_instr_->GetInstrList()->Insert(std::next(instr_it), new_instr);
                        //!attention: skip next new instr
                        ++instr_it;
                    }
                }
            }
            else if(typeid(*(*instr_it)) == typeid(assem::MoveInstr)){
                auto move_instr = static_cast<assem::MoveInstr*>(*instr_it);
                // If in use
                auto instr_use = move_instr->Use();
                if(instr_use != nullptr){
                    auto use_list = instr_use->GetList();
                    bool is_exist = false;
                    for(auto use : use_list){
                        if(use == v_temp){
                            is_exist = true;
                            break;
                        }
                    }
                    if(is_exist){
                        auto replace_temp = temp::TempFactory::NewTemp();
                        move_instr->src_ = temp::TempList::RewriteTempList(instr_use, v_temp, replace_temp);
                        // insert new instruction
                        std::string instr_assem = "movq (" + frame_->GetLabel() + "_framesize" + std::to_string(v_access->offset) + ")(`s0), `d0";
                        auto src_list = new temp::TempList();
                        auto dst_list = new temp::TempList();
                        src_list->Append(reg_manager->StackPointer());
                        dst_list->Append(replace_temp);
                        assem::Instr* new_instr = new assem::OperInstr(
                            instr_assem,
                            dst_list,
                            src_list,
                            nullptr
                        );
                        assem_instr_->GetInstrList()->Insert(instr_it, new_instr);
                    }
                }

                // If in def
                auto instr_def = move_instr->Def();
                if(instr_def != nullptr){
                    auto def_list = instr_def->GetList();
                    bool is_exist = false;
                    for(auto def : def_list){
                        if(def == v_temp){
                            is_exist = true;
                            break;
                        }
                    }
                    if(is_exist){
                        auto replace_temp = temp::TempFactory::NewTemp();
                        move_instr->dst_ = temp::TempList::RewriteTempList(instr_def, v_temp, replace_temp);
                        std::string instr_assem = "movq `s0, (" + frame_->GetLabel() + "_framesize" + std::to_string(v_access->offset) + ")(`s1)";
                        auto src_list = new temp::TempList();
                        src_list->Append(replace_temp);
                        src_list->Append(reg_manager->StackPointer());
                        assem::Instr* new_instr = new assem::OperInstr(
                            instr_assem,
                            nullptr,
                            src_list,
                            nullptr
                        );
                        assem_instr_->GetInstrList()->Insert(std::next(instr_it), new_instr);
                        //!attention: skip next new instr
                        ++instr_it;
                    }
                }
            }

        }
    }
}

} // namespace ra