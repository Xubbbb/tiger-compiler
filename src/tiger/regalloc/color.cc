#include "tiger/regalloc/color.h"
#include <iostream>

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */

col::Result Color::RAColor(){
    Build();
    MakeWorklist();
    while(!(simplifyWorklist->GetList().empty() && worklistMoves->GetList().empty() && freezeWorklist->GetList().empty() && spillWorklist->GetList().empty())){
        if(!simplifyWorklist->GetList().empty()){
            Simplify();
        }
        else if(!worklistMoves->GetList().empty()){
            Coalesce();
        }
        else if(!freezeWorklist->GetList().empty()){
            Freeze();
        }
        else if(!spillWorklist->GetList().empty()){
            SelectSpill();
        }
    }
    AssignColors();
    if(!spilledNodes->GetList().empty()){
        auto res = col::Result();
        res.spills = spilledNodes;
        return res;
    }
    else{
        auto res = col::Result();
        res.coloring = temp::Map::Empty();
        auto node_list = ig->Nodes()->GetList();
        for(auto node : node_list){
            auto temp = node->NodeInfo();
            auto node_color = color->Look(node);
            auto color_str = reg_manager->temp_map_->Look(node_color);
            res.coloring->Enter(node->NodeInfo(), color_str);
        }
        return res;
    }
}

void Color::Build(){
    flow_fac = new fg::FlowGraphFactory(instr_list);
    flow_fac->AssemFlowGraph();
    // auto fg_fac = fg::FlowGraphFactory(instr_list);
    // fg_fac.AssemFlowGraph();
    live_fac = new live::LiveGraphFactory(flow_fac->GetFlowGraph());
    live_fac->Liveness();
    // auto live_fac = live::LiveGraphFactory(fg_fac.GetFlowGraph());
    // live_fac.Liveness();
    auto lg = live_fac->GetLiveGraph();
    ig = lg.interf_graph;
    precolored = new live::INodeList();
    simplifyWorklist = new live::INodeList();
    freezeWorklist = new live::INodeList();
    spillWorklist = new live::INodeList();
    spilledNodes = new live::INodeList();
    coalescedNodes = new live::INodeList();
    coloredNodes = new live::INodeList();
    selectStack = new live::INodeList();

    coalescedMoves = new live::MoveList();
    constrainedMoves = new live::MoveList();
    frozenMoves = new live::MoveList();
    worklistMoves = lg.moves;
    activeMoves = new live::MoveList();

    degree = new graph::Table<temp::Temp, int>();
    moveList = new graph::Table<temp::Temp, live::MoveList>();
    alias = new graph::Table<temp::Temp, live::INode>();
    color = new graph::Table<temp::Temp, temp::Temp>();

    auto reg_list = reg_manager->Registers()->GetList();
    auto node_map = live_fac->GetTempNodeMap();
    for(auto reg : reg_list){
        auto reg_node = node_map->Look(reg);
        precolored->Append(reg_node);
        coloredNodes->Append(reg_node);
        color->Enter(reg_node, reg);
    }

    auto node_list = ig->Nodes()->GetList();
    for(auto node : node_list){
        auto node_moveList = new live::MoveList();
        auto allMoves_list = lg.moves->GetList();
        for(auto move_rel : allMoves_list){
            if(move_rel.first == node || move_rel.second == node){
                if(!node_moveList->Contain(move_rel.first, move_rel.second)){
                    node_moveList->Append(move_rel.first, move_rel.second);
                }
            }
        }
        moveList->Enter(node, node_moveList);
        int *node_degree = new int(0);
        if(precolored->Contain(node)){
            /**
             * If a node is precolored, we should set its degree to a large number
            */
            *node_degree = 1000;
        }
        else{
            *node_degree = node->OutDegree();
        }
        degree->Enter(node, node_degree);
    }
}

void Color::AddEdge(live::INodePtr u, live::INodePtr v){
    if(!u->Succ()->Contain(v) && u != v){
        ig->AddEdge(u, v);
        ig->AddEdge(v, u);
        auto u_degree_ptr = degree->Look(u);
        auto v_degree_ptr = degree->Look(v);
        auto u_value = *u_degree_ptr;
        auto v_value = *v_degree_ptr;
        *u_degree_ptr = u_value + 1;
        *v_degree_ptr = v_value + 1;
    }
}

void Color::MakeWorklist(){
    auto node_list = ig->Nodes()->GetList();
    for(auto node : node_list){
        if(!precolored->Contain(node)){
            if(*(degree->Look(node)) >= K){
                spillWorklist->Append(node);
            }
            else if(MoveRelated(node)){
                freezeWorklist->Append(node);
            }
            else{
                simplifyWorklist->Append(node);
            }
        }
    }
}

live::INodeListPtr Color::Adjacent(live::INodePtr n){
    auto adjList = n->Succ();
    auto select_union_coalesced = selectStack->Union(coalescedNodes);
    auto res = adjList->Diff(select_union_coalesced);

    return res;
}

/**
 * Construct node's related movelist
*/
live::MoveList* Color::NodeMoves(live::INodePtr n){
    auto moveList_n = moveList->Look(n);
    auto active_union_work = activeMoves->Union(worklistMoves);
    auto res = moveList_n->Intersect(active_union_work);

    return res;
}

bool Color::MoveRelated(live::INodePtr n){
    auto node_moves = NodeMoves(n);
    return !node_moves->GetList().empty();
}
/**
 * Simulation removes a low-degree node from the graph
 * and decrement its adjacents
*/
void Color::Simplify(){
    auto n = simplifyWorklist->GetList().front();
    simplifyWorklist->DeleteNode(n);
    //! maybe wrong
    selectStack->Prepend(n);
    auto adj_list = Adjacent(n)->GetList();
    for(auto m : adj_list){
        DecrementDegree(m);
    }
}

/**
 * If we remove a node, we should decrease this 
 * node's every adjacent nodes' degree
*/
void Color::DecrementDegree(live::INodePtr m){
    auto d_ptr = degree->Look(m);
    auto d_initial_value = *d_ptr;
    *d_ptr = d_initial_value - 1;
    if(d_initial_value == K){
        auto m_union_adj = Adjacent(m);
        m_union_adj->Append(m);
        /**
         * If a node's degree decrease to K-1 its move related instruction
         * (if exist) will become possible to coalesce
        */
        EnableMoves(m_union_adj);
        spillWorklist->DeleteNode(m);
        if(MoveRelated(m)){
            freezeWorklist->Append(m);
        }
        else{
            simplifyWorklist->Append(m);
        }
    }
}

/**
 * let nodes' related moves become possible to coalesce
*/
void Color::EnableMoves(live::INodeListPtr nodes){
    auto nodes_list = nodes->GetList();
    for(auto n : nodes_list){
        auto node_moves = NodeMoves(n);
        auto node_moves_list = node_moves->GetList();
        for(auto m : node_moves_list){
            //! maybe wrong
            if(activeMoves->Contain(m.first, m.second)){
                activeMoves->Delete(m.first, m.second);
                if(!worklistMoves->Contain(m.first, m.second)){
                    worklistMoves->Append(m.first, m.second);
                }
            }
        }
    }
}

void Color::Coalesce(){
    auto worklistMoves_list = worklistMoves->GetList();
    auto m = worklistMoves_list.front();
    auto x = GetAlias(m.first);
    auto y = GetAlias(m.second);
    live::INodePtr u = nullptr;
    live::INodePtr v = nullptr;
    if(precolored->Contain(y)){
        u = y;
        v = x;
    }
    else{
        u = x;
        v = y;
    }
    worklistMoves->Delete(m.first, m.second);
    if(u == v){
        if(!coalescedMoves->Contain(m.first, m.second)){
            coalescedMoves->Append(m.first, m.second);
        }
        AddWorkList(u);
    }
    else if(precolored->Contain(v) || u->Succ()->Contain(v)){
        //!maybe wrong
        if(!constrainedMoves->Contain(m.first, m.second)){
            constrainedMoves->Append(m.first, m.second);
        }
        AddWorkList(u);
        AddWorkList(v);
    }
    else if((precolored->Contain(u) && OK(v, u)) || (!precolored->Contain(u) && Conservative(Adjacent(u)->Union(Adjacent(v))))){
        //!maybe wrong
        if(!coalescedMoves->Contain(m.first, m.second)){
            coalescedMoves->Append(m.first, m.second);
        }
        Combine(u, v);
        AddWorkList(u);
    }
    else{
        if(!activeMoves->Contain(m.first, m.second)){
            activeMoves->Append(m.first, m.second);
        }
    }
}

/**
 * If we give up to coalesce some node
 * and this node is a low-degree node
 * we should move it from freezeWorkList
 * to simpifyWorkList
*/
void Color::AddWorkList(live::INodePtr u){
    if(!precolored->Contain(u) && !MoveRelated(u) && (*(degree->Look(u)) < K)){
        freezeWorklist->DeleteNode(u);
        if(!simplifyWorklist->Contain(u)){
            simplifyWorklist->Append(u);
        }
    }
}

/**
 * George
*/
//! This OK function is a little different from book's.
//! For convenience, we add adjacent loop into function
bool Color::OK(live::INodePtr t, live::INodePtr r){
    auto adj_list = Adjacent(t)->GetList();
    for(auto adj : adj_list){
        if(!((*(degree->Look(adj)) < K) || (precolored->Contain(adj)) || (adj->Succ()->Contain(r)))){
            return false;
        }
    }
    return true;
}
/**
 * Briggs
*/
bool Color::Conservative(live::INodeListPtr nodes){
    int k = 0;
    auto node_list = nodes->GetList();
    for(auto n : node_list){
        if(*(degree->Look(n)) >= K){
            k = k + 1;
        }
    }
    return k < K;
}

live::INodePtr Color::GetAlias(live::INodePtr n){
    if(coalescedNodes->Contain(n)){
        return GetAlias(alias->Look(n));
    }
    return n;
}

void Color::Combine(live::INodePtr u, live::INodePtr v){
    if(freezeWorklist->Contain(v)){
        freezeWorklist->DeleteNode(v);
    }
    else{
        spillWorklist->DeleteNode(v);
    }
    coalescedNodes->Append(v);
    alias->Enter(v, u);
    auto moveList_u = moveList->Look(u);
    auto moveList_v = moveList->Look(v);
    auto moveList_u_union_v = moveList_u->Union(moveList_v);

    moveList->Set(u, moveList_u_union_v);
    auto v_list = new live::INodeList();
    v_list->Append(v);
    EnableMoves(v_list);

    auto adjacent = Adjacent(v);
    auto adj_list = adjacent->GetList();
    for(auto t : adj_list){
        AddEdge(t, u);
        DecrementDegree(t);
    }
    if(*(degree->Look(u)) >= K && freezeWorklist->Contain(u)){
        freezeWorklist->DeleteNode(u);
        if(!spillWorklist->Contain(u)){
            spillWorklist->Append(u);
        }
    }
}

/**
 * When all nodes which can be coalesced are
 * handled, we should let some move freeze
*/
void Color::Freeze(){
    auto u = freezeWorklist->GetList().front();
    freezeWorklist->DeleteNode(u);
    if(!simplifyWorklist->Contain(u)){
        simplifyWorklist->Append(u);
    }
    FreezeMoves(u);
}

void Color::FreezeMoves(live::INodePtr u){
    auto node_moves = NodeMoves(u);
    auto node_moves_list = node_moves->GetList();
    for(auto m : node_moves_list){
        auto x = m.first;
        auto y = m.second;
        live::INodePtr v = nullptr;
        if(GetAlias(y) == GetAlias(u)){
            v = GetAlias(x);
        }
        else{
            v = GetAlias(y);
        }
        activeMoves->Delete(x, y);
        if(!frozenMoves->Contain(x, y)){
            frozenMoves->Append(x, y);
        }
        auto node_moves_v = NodeMoves(v);
        auto node_moves_list_v = node_moves_v->GetList();
        if(node_moves_list_v.empty() && *(degree->Look(v)) < K){
            freezeWorklist->DeleteNode(v);
            if(!simplifyWorklist->Contain(v)){
                simplifyWorklist->Append(v);
            }
        }
    }
}

/**
 * try letting a high-degree node remove from graph
*/
void Color::SelectSpill(){
    auto m = spillWorklist->GetList().front();
    spillWorklist->DeleteNode(m);
    if(!simplifyWorklist->Contain(m)){
        simplifyWorklist->Append(m);
    }
    FreezeMoves(m);
}

void Color::AssignColors(){
    auto selectStack_list = selectStack->GetList();
    for(auto n : selectStack_list){
        auto okColors = reg_manager->Registers();
        auto adjList_n = n->Succ()->GetList();
        auto colored_union_precolored = coloredNodes->Union(precolored);
        for(auto w : adjList_n){
            auto w_alias = GetAlias(w);
            if(colored_union_precolored->Contain(w_alias)){
                auto temp_colored = new temp::TempList();
                temp_colored->Append(color->Look(w_alias));
                okColors = temp::TempList::DiffTempList(okColors, temp_colored);
            }
        }
        if(okColors->GetList().empty()){
            if(!spilledNodes->Contain(n)){
                spilledNodes->Append(n);
            }
        }
        else{
            if(!coloredNodes->Contain(n)){
                coloredNodes->Append(n);
            }
            auto c = okColors->GetList().front();
            color->Enter(n, c);
        }
    }
    selectStack->Clear();
    auto coalesced_list = coalescedNodes->GetList();
    for(auto n : coalesced_list){
        auto alias_color = color->Look(GetAlias(n));
        color->Enter(n, alias_color);
    }
}




} // namespace col
