#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result()
{
    if(coloring_) delete coloring_;
    if(il_) delete il_;
}

RegAllocator::RegAllocator(frame::Frame* frame_,std::unique_ptr<cg::AssemInstr> assem_instr_):frame(frame_),assem_instr(assem_instr_.get())
{
    flow_graph_factory=new fg::FlowGraphFactory(assem_instr.get()->GetInstrList());
    flow_graph_factory->AssemFlowGraph();
    live_graph_factory=new live::LiveGraphFactory(flow_graph_factory->GetFlowGraph());
}

void RegAllocator::Main(frame::Frame* f,assem::InstrList* instrs)
{
    while(true)
    {
        live_graph_factory->Liveness();
        auto live_graph=live_graph_factory->GetLiveGraph();
        Build();
        MakeWorklist();

        while(true)
        {
            if(simplifyWorklist->GetList().size()!=0) Simplify();
            if(worklistMoves->GetList().size()!=0) Coalesce();
            if(freezeWorklist->GetList().size()!=0) Freeze();
            if(spillWorklist->GetList().size()!=0) SelectSpill();
            if((simplifyWorklist->GetList().size()==0)
                &&(worklistMoves->GetList().size()==0)
                &&(freezeWorklist->GetList().size()==0)
                &&(spillWorklist->GetList().size()==0))
            {break;}
        }
        AssignColors();
        if(spilledNodes->GetList().size()!=0)
        {
            RewriteProgram(f,instrs);
        }
        else
        {
            break;
        }
        
    }
    result=new Result(ToMap(color),instrs);
}

void RegAllocator::Build()
{

    selectStack=new live::INodeList();
    coalescedMoves=new live::MoveList();
    constrainedMoves=new live::MoveList();
    frozenMoves=new live::MoveList();
    auto live_graph=live_graph_factory->GetLiveGraph();
    worklistMoves=live_graph.moves;
    activeMoves=new live::MoveList();

    degree=new std::unordered_map<live::INodePtr,int>();
    moveList=new std::unordered_map<live::INodePtr,live::MoveList*>();
    alias=new std::unordered_map<live::INodePtr,live::INodePtr>();
    color=new std::unordered_map<live::INodePtr,std::string*>();
    precolored=new std::set<live::INodePtr>();

    std::list<live::INodePtr> node_list = live_graph.interf_graph->Nodes()->GetList();
    for(const auto node:node_list)
    {
        degree->insert(std::make_pair(node,node->OutDegree()));
        color->insert(std::make_pair(node,temp::Map::Name()->Look(node->NodeInfo())));
        alias->insert(std::make_pair(node,node));

        live::MoveList* movelist=new live::MoveList();
        for(const auto move:worklistMoves->GetList())
        {
            if(move.first==node||move.second==node)
            {
                movelist->Append(move.first,move.second);
            }
        }
        moveList->insert(std::make_pair(node,movelist));
    }
}

void RegAllocator::MakeWorklist()
{
    auto live_graph=live_graph_factory->GetLiveGraph();
    std::list<live::INodePtr> nodes = live_graph.interf_graph->Nodes()->GetList();
    for(const auto n:nodes)
    {
        if(precolored->count(n)) continue;
        if((*degree)[n]>=reg_manager->GetK())
        {
            spillWorklist->Append(n);
        }
        else if(MoveRelated(n))
        {
            freezeWorklist->Append(n);
        }
        else
        {
            simplifyWorklist->Append(n);
        }
    }
}

void RegAllocator::Simplify()
{
    live::INodePtr n = simplifyWorklist->PopAndRemove();
    selectStack->Append(n);
    live::INodeListPtr adj_nodes = n->Adj();
    std::list<live::INodePtr> adj_nodes_list = adj_nodes->GetList();
    for(const auto node:adj_nodes_list)
    {
        DecrementDegree(node);
    }
}

void RegAllocator::DecrementDegree(live::INodePtr m)
{
    int d = (*degree)[m];
    (*degree)[m]=d-1;
    int k=reg_manager->GetK();
    if(d==k)
    {
        live::INodeListPtr m_list = new live::INodeList();
        m_list->Append(m);
        live::INodeListPtr union_list = Adjacent(m)->Union(m_list);
        EnableMoves(union_list);
        spillWorklist->DeleteNode(m);

        if(MoveRelated(m)){
            freezeWorklist->Append(m);
        }else{
            spillWorklist->Append(m);
        }
    }

}

void RegAllocator::EnableMoves(live::INodeListPtr nodes)
{
    std::list<live::INodePtr> nodes_list = (*nodes).GetList();
    for(const auto n:nodes_list)
    {
        std::list<live::MovePtrs> m_list = NodeMoves(n)->GetList();
        for(const auto m:m_list)
        {
            if(activeMoves->Contain(m.first,m.second))
            {
                activeMoves->Delete(m.first,m.second);
                worklistMoves->Append(m.first,m.second);
            }
        }
    }
}

live::MoveList* RegAllocator::NodeMoves(live::INodePtr n)
{
    auto live_graph=live_graph_factory->GetLiveGraph();
    live::MoveList* move_related = live_graph.moves->GetRelatedMove(n);
    return move_related->Intersect(activeMoves->Union(worklistMoves));
}

bool RegAllocator::MoveRelated(live::INodePtr n)
{
    live::MoveList* moves = NodeMoves(n);
    return moves->GetList().size()!=0;
}

void RegAllocator::Coalesce()
{
    live::MovePtrs m = worklistMoves->GetList().back();
    live::INodePtr x =GetAlias(m.first);
    live::INodePtr y =GetAlias(m.second);
    live::INodePtr u,v;
    if(precolored->count(y))
    {
        u=y;
        v=x;
    }
    else
    {
        u=x;
        v=y;
    }
    worklistMoves->Delete(m.first,m.second);

    bool flag=false;
    std::list<live::INodePtr> adj_list = Adjacent(v)->GetList();
    for(const auto adj:adj_list)
    {
        if(OK(adj,u)){
            flag=true;
            break;
        }
    }

    if(u==v)
    {
        coalescedMoves->Append(m.first,m.second);
        AddWorkList(u);
    }
    else if(precolored->count(v)
        || v->Succ()->Contain(u))
    {
        constrainedMoves->Append(m.first,m.second);
        AddWorkList(u);
        AddWorkList(v);
    }
    else if((precolored->count(u) && flag)
     || (!precolored->count(v) && Conservative(Adjacent(u)->Union(Adjacent(v)))))
    {
        coalescedMoves->Append(m.first,m.second);
        Combine(u,v);
        AddWorkList(u);
    }
    else{
        activeMoves->Append(m.first,m.second);
    }
}

void RegAllocator::AddWorkList(live::INodePtr n)
{
    if((!precolored->count(n))
        &&(!MoveRelated(n))
        &&(n->Degree()<reg_manager->GetK()))
    {
        freezeWorklist->DeleteNode(n);
        simplifyWorklist->DeleteNode(n);
    }   
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n)
{
    if(coalescedNodes->Contain(n))
    {
        return GetAlias((*(alias->find(n))).second);
    }
    else{
        return n;
    }
}

bool RegAllocator::OK(live::INodePtr t,live::INodePtr r)
{
    return (t->Degree()<reg_manager->GetK())
        ||(precolored->count(t)
        ||(r->Succ()->Contain(t)));
}

void RegAllocator::Combine(live::INodePtr u,live::INodePtr v)
{
    if(freezeWorklist->Contain(v))
    {
        freezeWorklist->DeleteNode(v);
    }else{
        spillWorklist->DeleteNode(v);
    }
    coalescedNodes->Append(v);
    (*alias)[v]=u;
    live::MoveList* u_list = (*(moveList->find(u))).second;
    live::MoveList* v_list = (*(moveList->find(v))).second;
    u_list = u_list->Union(v_list);
    (*moveList)[u]=u_list;
    
    live::INodeListPtr enable_list = new live::INodeList();
    enable_list->Append(v);
    EnableMoves(enable_list);
    std::list<live::INodePtr> t_list = v->Adj()->GetList();
    for(const auto t:t_list)
    {
        AddEdge(t,u);
        DecrementDegree(t);
    }
    if(((*degree)[u]>=reg_manager->GetK())
        &&(freezeWorklist->Contain(u)))
    {
        freezeWorklist->DeleteNode(u);
        spillWorklist->Append(u);
    }
}

void RegAllocator::AddEdge(live::INodePtr u,live::INodePtr v)
{
    auto live_graph=live_graph_factory->GetLiveGraph();
    if(!u->Succ()->Contain(v) && u!=v)
    {
        live_graph.interf_graph->AddEdge(u,v);
        if(!precolored->count(u))
        {
            (*degree)[u]++;
        }
        if(!precolored->count(v))
        {
            (*degree)[v]++;
        }
    }
}

bool RegAllocator::Conservative(live::INodeListPtr node_list)
{
    int k=0;
    std::list<live::INodePtr> nodes = node_list->GetList();
    for(const auto n:nodes)
    {
        if((*degree)[n]>=reg_manager->GetK())
        {
            ++k;
        }
    }
    return (k<reg_manager->GetK());
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n)
{
    return n->Adj()->Diff(selectStack->Union(coloredNodes));
}

void RegAllocator::Freeze()
{
    live::INodePtr u = freezeWorklist->PopAndRemove();
    simplifyWorklist->Append(u);
    FreezeMoves(u);
}

void RegAllocator::FreezeMoves(live::INodePtr u)
{
    std::list<live::MovePtrs> m_list = NodeMoves(u)->GetList();
    for(const auto m:m_list)
    {
        live::INodePtr x = m.first, y = m.second,v;
        if(GetAlias(y)==GetAlias(u))
        {
            v=GetAlias(x);
        }else{
            v=GetAlias(y);
        }
        activeMoves->Delete(m.first,m.second);
        frozenMoves->Append(m.first,m.second);
        if((NodeMoves(v)->GetList().size()==0)
            && ((*degree)[v]<reg_manager->GetK()))
        {
            freezeWorklist->DeleteNode(v);
            simplifyWorklist->Append(v);
        }
    }
}

void RegAllocator::SelectSpill()
{
    // std::list<live::INodePtr> spill_list = spillWorklist->GetList();
    // for(const auto node:spill_list)
    // {
    //     if(noSpillingTemp.count(node->NodeInfo())) continue;

    // }
    //TODO:select proper to spill
    live::INodePtr spilled_node = spillWorklist->PopAndRemove();
    simplifyWorklist->Append(spilled_node);
    FreezeMoves(spilled_node);
}

void RegAllocator::AssignColors()
{
    while(selectStack->GetList().size()!=0)
    {
        live::INodePtr n = selectStack->PopAndRemove();
        std::unordered_map<std::string*,bool> okColors;
        std::list<temp::Temp *> reg_list=reg_manager->Registers()->GetList();
        temp::Temp * rsp=reg_manager->StackPointer();
        for(const auto reg:reg_list)
        {
            if(reg==rsp) continue;
            std::string* key=temp::Map::Name()->Look(reg);
            okColors[key]=true;
        }
        std::list<live::INodePtr> w_list = n->Adj()->GetList();
        for(const auto w:w_list)
        {
            if(coloredNodes->Union(SetToList(precolored))->Contain(GetAlias(w)))
            {
                okColors.erase((*color)[GetAlias(w)]);
            }
        }
        if(okColors.empty())
        {
            spilledNodes->Append(n);
        }
        else
        {
            coloredNodes->Append(n);
            auto c=okColors.begin();
            okColors.erase(c);
            (*color)[n]=(*c).first;
        }
    }
    std::list<live::INodePtr> n_list = coalescedNodes->GetList();
    for(const auto n:n_list)
    {
        (*color)[n]=(*color)[GetAlias(n)];
    }
}

live::INodeListPtr RegAllocator::SetToList(std::set<live::INodePtr>* set_)
{
    live::INodeListPtr ptr=new live::INodeList();
    for(auto it:*set_)
    {
        ptr->Append(it);
    }
    return ptr;
}

void RegAllocator::RewriteProgram(frame::Frame* f,assem::InstrList* instrs)
{
    noSpillingTemp->clear();
    std::list<live::INodePtr> spill_list = spilledNodes->GetList();
    for(auto it=spill_list.begin();it!=spill_list.end();++it)
    {
        live::INodePtr node=*it;
        temp::Temp * spilled_temp = node->NodeInfo();
        std::list<assem::Instr *> instrs_list = instrs->GetList();
        for(auto iter=instrs_list.begin();iter!=instrs_list.end();++iter)
        {
            auto instr=*iter;
            temp::TempList* temp_src=instr->Use();
            temp::TempList* temp_dst=instr->Def();
            if(temp_src&&Contain(temp_src,spilled_temp))
            {
                temp::Temp* new_temp = temp::TempFactory::NewTemp();
                noSpillingTemp->emplace(new_temp);
                for(auto& t:temp_src->GetRawList())
                {
                    if(t==spilled_temp)
                    {
                        t=new_temp;
                        break;
                    }
                }
                std::string instr_raw="movq ("+f->name->Name()+"_framesize-"+std::to_string(-f->offset)+")(`s0), `d0";
                instrs->Insert(iter,new assem::OperInstr(instr_raw,new temp::TempList(new_temp),new temp::TempList(reg_manager->StackPointer()),nullptr));
                instrs->Remove(instr);
            }

            if(temp_dst&&Contain(temp_dst,spilled_temp))
            {
                temp::Temp* new_temp = temp::TempFactory::NewTemp();
                noSpillingTemp->emplace(new_temp);
                for(auto& t:temp_src->GetRawList())
                {
                    if(t==spilled_temp)
                    {
                        t=new_temp;
                        break;
                    }
                }
                std::string instr_raw="movq `s0, ("+f->name->Name()+"_framesize-"+std::to_string(-f->offset)+")(`d0)";
                instrs->Insert(iter,new assem::OperInstr(instr_raw,new temp::TempList(new_temp),new temp::TempList(reg_manager->StackPointer()),nullptr));
                instrs->Remove(instr);
            }

        }
    }
}

bool RegAllocator::Contain(temp::TempList* temp_list,temp::Temp* temp)
{
    std::list<temp::Temp *> container = temp_list->GetList();
    for(const auto item:container)
    {
        if(item==temp) return true;
    }
    return false;
}

temp::Map* RegAllocator::ToMap(std::unordered_map<live::INodePtr,std::string*>* color)
{
    temp::Map* color_map=temp::Map::Empty();
    auto origin_map=*color;
    for(const auto item:origin_map)
    {
        color_map->Enter(item.first->NodeInfo(),item.second);
    }
    return color_map;
}

} // namespace ra