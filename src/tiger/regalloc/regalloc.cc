#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

void ShowList(live::INodeListPtr nodelist)
{
    std::list<live::INodePtr> nodes=nodelist->GetList();
    for(const auto node:nodes)
    {
        std::cout<<node->NodeInfo()->Int()<<" ";
    }
    std::cout<<std::endl;
}

void ShowMoveList(live::MoveList* movelist)
{
    auto moves = movelist->GetList();
    for(const auto mv:moves)
    {
        std::cout<<"("<<mv.first->NodeInfo()->Int()<<","<<mv.second->NodeInfo()->Int()<<") ";
    }
    std::cout<<std::endl;
}

void ShowInstrInfo(assem::Instr* instr)
{
  if(typeid(*instr)==typeid(assem::OperInstr))
  {
    std::cout<<"oper ";
    std::cout<<((assem::OperInstr*)instr)->assem_<<std::endl;
  }
  else if(typeid(*instr)==typeid(assem::MoveInstr))
  {
    std::cout<<"move ";
    std::cout<<((assem::MoveInstr*)instr)->assem_<<std::endl;
  }
  else if(typeid(*instr)==typeid(assem::LabelInstr))
  {
    std::cout<<"label ";
    std::cout<<((assem::LabelInstr*)instr)->assem_<<std::endl;
  }
  else
  {
    std::cout<<"error type"<<std::endl;
  }
}

void ShowInstrList(assem::InstrList *instr_list)
{
    std::cout<<"------------program------------"<<std::endl;
    std::cout<<"size:"<<instr_list->GetList().size()<<std::endl;
    int cnt=1;
    for(const auto instr:instr_list->GetList())
    {
        std::cout<<cnt<<" ";cnt++;
        ShowInstrInfo(instr);
    }
    std::cout<<"--------------------------------"<<std::endl;
}

Result::~Result()
{
    if(coloring_) delete coloring_;
    if(il_) delete il_;
}

RegAllocator::RegAllocator(frame::Frame* frame_,cg::AssemInstr* assem_instr_):frame(frame_)
{
    assem_instr=assem_instr_;
    // std::cout<<"assem:"<<assem_instr<<std::endl;
    // std::cout<<"construct initial:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test

    // flow_graph_factory=new fg::FlowGraphFactory(assem_instr->GetInstrList());
    // flow_graph_factory->AssemFlowGraph();
    // live_graph_factory=new live::LiveGraphFactory(flow_graph_factory->GetFlowGraph());

    // std::cout<<"end construct initial:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
    // std::cout<<"instr list:"<<assem_instr->GetInstrList()<<std::endl;//test
}

void RegAllocator::Main()
{
    // std::cout<<"assem:"<<assem_instr<<std::endl;
    // std::cout<<"instrs initial:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
    std::cout<<"instr list begin size:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
    int cnt=0;
    while(true)
    {
        std::cout<<"try:"<<++cnt<<std::endl;//test

        flow_graph_factory=new fg::FlowGraphFactory(assem_instr->GetInstrList());
        flow_graph_factory->AssemFlowGraph();
        live_graph_factory=new live::LiveGraphFactory(flow_graph_factory->GetFlowGraph());

        live_graph_factory->Liveness();
        auto live_graph=live_graph_factory->GetLiveGraph();
        Build();
        MakeWorklist();

        while(true)
        {
            std::cout<<"try in"<<std::endl;//test
            if(simplifyWorklist->GetList().size()!=0) Simplify();
            else if(worklistMoves->GetList().size()!=0) Coalesce();
            else if(freezeWorklist->GetList().size()!=0) Freeze();
            else if(spillWorklist->GetList().size()!=0) SelectSpill();

            if((simplifyWorklist->GetList().size()==0)
                &&(worklistMoves->GetList().size()==0)
                &&(freezeWorklist->GetList().size()==0)
                &&(spillWorklist->GetList().size()==0))
            {break;}
        }
        AssignColors();
        if(spilledNodes->GetList().size()!=0)
        {
            // std::cout<<"instrs before rewrite:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
            RewriteProgram();
            // std::cout<<"instrs after rewrite:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
        }
        else
        {
            break;
        }
        
    }
    temp::Map* res=temp::Map::Empty();
    auto nodes = live_graph_factory->GetLiveGraph().interf_graph->Nodes()->GetList();
    std::cout<<"nodes:"<<nodes.size()<<std::endl;//test
    for(const auto n:nodes)
    {
        res->Enter(n->NodeInfo(),(*color->find(n)).second);
    }
    std::cout<<"instrs end main:"<<assem_instr->GetInstrList()->GetList().size()<<std::endl;//test
    result=new Result(res,assem_instr->GetInstrList());
}

void RegAllocator::Build()
{
    // conflict_graph=new live::IGraph();
    // gdb -args ./build/tiger-compiler ./testdata/lab5or6/testcases/bsearch.tig

    simplifyWorklist=new live::INodeList();
    freezeWorklist=new live::INodeList();
    spillWorklist=new live::INodeList();

    selectStack=new live::INodeList();
    coalescedMoves=new live::MoveList();
    constrainedMoves=new live::MoveList();
    frozenMoves=new live::MoveList();
    auto live_graph=live_graph_factory->GetLiveGraph();
    worklistMoves=live_graph.moves;
    std::cout<<"worklist moves initial size:"<<worklistMoves->GetList().size()<<std::endl;
    ShowMoveList(worklistMoves);
    activeMoves=new live::MoveList();

    spilledNodes=new live::INodeList();
    coalescedNodes=new live::INodeList();
    coloredNodes=new live::INodeList();

    precolored=new std::set<temp::Temp*>();
    noSpillingTemp=new std::set<temp::Temp*>();

    degree=new std::unordered_map<live::INodePtr,int>();
    moveList=new std::unordered_map<live::INodePtr,live::MoveList*>();
    alias=new std::unordered_map<live::INodePtr,live::INodePtr>();
    color=new std::unordered_map<live::INodePtr,std::string*>();
    // color=new std::set<live::INodePtr>();

    // precolored initialize
    std::list<temp::Temp *> regs = reg_manager->Registers()->GetList();
    for(const auto reg:regs)
    {
        precolored->insert(reg);
    }

    std::list<live::INodePtr> node_list = live_graph.interf_graph->Nodes()->GetList();
    for(const auto node:node_list)
    {
        std::cout<<"origin neighbour of:"<<node->NodeInfo()->Int()<<" " <<node->Adj()->GetList().size()<<std::endl;
        auto test_list=node->Adj()->GetList();
        for(const auto t:test_list)
        {
            std::cout<<t->NodeInfo()->Int()<<" ";
        }
        std::cout<<std::endl;


        degree->insert(std::make_pair(node,node->Degree()));
        color->insert(std::make_pair(node,temp::Map::Name()->Look(node->NodeInfo())));
        alias->insert(std::make_pair(node,node));
    }

    for(auto mv:live_graph.moves->GetList())
    {
        (*moveList)[mv.first]=new live::MoveList();
        (*moveList)[mv.first]->Append(mv.first,mv.second);
        (*moveList)[mv.second]=new live::MoveList();
        (*moveList)[mv.second]->Append(mv.first,mv.second);
    }
}

void RegAllocator::MakeWorklist()
{
    auto live_graph=live_graph_factory->GetLiveGraph();
    std::list<live::INodePtr> nodes = live_graph.interf_graph->Nodes()->GetList();
    for(const auto n:nodes)
    {
        std::cout<<"make:"<<n->NodeInfo()->Int()<<" ";
        if((*degree)[n]>=reg_manager->GetK())
        {
            std::cout<<" spillworklist"<<std::endl;
            spillWorklist->Append(n);
        }
        else if(MoveRelated(n))
        {
            std::cout<<" freezeworklist"<<std::endl;
            freezeWorklist->Append(n);
        }
        else
        {
            std::cout<<" simplifyworklist"<<std::endl;
            simplifyWorklist->Append(n);
        }
    }
}

void RegAllocator::Simplify()
{
    std::cout<<"simplify"<<std::endl;
    ShowList(simplifyWorklist);
    live::INodePtr n = simplifyWorklist->PopAndRemove();
    ShowList(simplifyWorklist);
    selectStack->Append(n);
    std::cout<<"select stack add:"<<n->NodeInfo()->Int()<<std::endl;
    live::INodeListPtr adj_nodes = n->Adj();
    std::list<live::INodePtr> adj_nodes_list = adj_nodes->GetList();
    // adj_nodes->DeleteNode(n);
    for(const auto node:adj_nodes_list)
    {
        DecrementDegree(node);
    }
}

void RegAllocator::DecrementDegree(live::INodePtr m)
{
    int d=(*degree)[m];
    (*degree)[m]--;
    int k=reg_manager->GetK();
    // degree has already been decreased
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
                std::cout<<"worklist append:";
                ShowMoveList(worklistMoves);
                worklistMoves->Append(m.first,m.second);
                ShowMoveList(worklistMoves);
            }
        }
    }
}

live::MoveList* RegAllocator::NodeMoves(live::INodePtr n)
{
    live::MoveList* move_related = (*moveList)[n];
    if(move_related==nullptr)
    {
        return new live::MoveList();
    }
    return move_related->Intersect(activeMoves->Union(worklistMoves));
}

bool RegAllocator::MoveRelated(live::INodePtr n)
{
    live::MoveList* moves = NodeMoves(n);
    return moves->GetList().size()!=0;
}

void RegAllocator::Coalesce()
{
    std::cout<<"coalesce"<<std::endl;
    live::MovePtrs m = worklistMoves->GetList().back();
    live::INodePtr x =GetAlias(m.first);
    live::INodePtr y =GetAlias(m.second);
    live::INodePtr u,v;
    if(precolored->count(y->NodeInfo()))
    {
        u=y;
        v=x;
    }
    else
    {
        u=x;
        v=y;
    }
    ShowMoveList(worklistMoves);
    worklistMoves->Delete(m.first,m.second);
    ShowMoveList(worklistMoves);

    bool flag=true;
    std::list<live::INodePtr> adj_list = Adjacent(v)->GetList();
    for(const auto adj:adj_list)
    {
        if(!OK(adj,u)){
            flag=false;
            break;
        }
    }

    if(u==v)
    {
        std::cout<<"coal choice 1"<<std::endl;
        coalescedMoves->Append(m.first,m.second);
        AddWorkList(u);
    }
    else if(precolored->count(v->NodeInfo())
        || v->Succ()->Contain(u)||u->Succ()->Contain(v))
    {
        std::cout<<"coal choice 2"<<std::endl;
        constrainedMoves->Append(m.first,m.second);
        AddWorkList(u);
        AddWorkList(v);
    }
    else if( (precolored->count(u->NodeInfo()) && flag)
     || (!precolored->count(u->NodeInfo()) && Conservative(Adjacent(u)->Union(Adjacent(v)))))
    {
        std::cout<<"coal choice 3"<<std::endl;
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
    // book p180 is wrong precolored->color
    if((!color->count(n))
        &&(!MoveRelated(n))
        &&((*degree)[n]<reg_manager->GetK()))
    {
        freezeWorklist->DeleteNode(n);
        std::cout<<"delete from freeze, add to simplify:"<<n->NodeInfo()->Int()<<std::endl;
        simplifyWorklist->Append(n);
    }   
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n)
{
    if(coalescedNodes->Contain(n))
    {
        return GetAlias((*alias)[n]);
    }
    else{
        return n;
    }
}

bool RegAllocator::OK(live::INodePtr t,live::INodePtr r)
{
    return (t->Degree()<reg_manager->GetK())
        ||(precolored->count(t->NodeInfo())
        ||(r->Succ()->Contain(t)));
}

void RegAllocator::Combine(live::INodePtr u,live::INodePtr v)
{
    std::cout<<"combine:"<<u->NodeInfo()->Int()<<" "<<v->NodeInfo()->Int()<<std::endl;
    if(freezeWorklist->Contain(v))
    {
        std::cout<<"freeze list delete:"<<v->NodeInfo()->Int()<<std::endl;
        freezeWorklist->DeleteNode(v);
    }else{
        std::cout<<"spill work list delete:"<<v->NodeInfo()->Int()<<std::endl;
        spillWorklist->DeleteNode(v);
    }
    std::cout<<"coalesce list append:"<<v->NodeInfo()->Int()<<std::endl;
    coalescedNodes->Append(v);
    (*alias)[v]=u;
    live::MoveList* u_list = (*moveList)[u];
    live::MoveList* v_list = (*moveList)[v];
    u_list = u_list->Union(v_list);
    (*moveList)[u]=u_list;
    
    live::INodeListPtr enable_list = new live::INodeList();
    enable_list->Append(v);
    EnableMoves(enable_list);
    std::list<live::INodePtr> t_list = Adjacent(v)->GetList();
    std::cout<<"adj of:"<<v->NodeInfo()->Int()<<" size:"<<t_list.size()<<std::endl;
    for(const auto t:t_list)
    {
        std::cout<<t->NodeInfo()->Int()<<std::endl;
        AddEdge(t,u);
        DecrementDegree(t);
    }
    if(((*degree)[u]>=reg_manager->GetK())
        &&(freezeWorklist->Contain(u)))
    {
        freezeWorklist->DeleteNode(u);
        std::cout<<"freeze list delete,spill work list add:"<<u->NodeInfo()->Int()<<std::endl;
        spillWorklist->Append(u);
    }
}

void RegAllocator::AddEdge(live::INodePtr u,live::INodePtr v)
{
    std::cout<<"add edge in regalloc:"<<u->NodeInfo()->Int()<<" "<<v->NodeInfo()->Int()<<std::endl;
    auto live_graph=live_graph_factory->GetLiveGraph();
    if(!u->Succ()->Contain(v) && u!=v)
    {
        live_graph.interf_graph->AddEdge(u,v);
        live_graph.interf_graph->AddEdge(v,u);
        (*degree)[u]++;
        (*degree)[v]++;
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
    return n->Adj()->Diff(selectStack->Union(coalescedNodes));
}

void RegAllocator::Freeze()
{
    std::cout<<"freeze"<<std::endl;
    live::INodePtr u = freezeWorklist->PopAndRemove();
    std::cout<<"freeze list delete, simplfy worklist add:"<<u->NodeInfo()->Int()<<std::endl;
    simplifyWorklist->Append(u);
    FreezeMoves(u);
}

void RegAllocator::FreezeMoves(live::INodePtr u)
{
    std::cout<<"freeze move"<<std::endl;
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
            std::cout<<"freeze list delete, simplify work list add:"<<v->NodeInfo()->Int()<<std::endl;
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
    std::cout<<"select spill"<<std::endl;
    live::INodePtr spilled_node = spillWorklist->PopAndRemove();
    simplifyWorklist->Append(spilled_node);
    FreezeMoves(spilled_node);
}

void RegAllocator::AssignColors()
{
    std::cout<<"assign color"<<std::endl;
    while(selectStack->GetList().size()!=0)
    {
        std::cout<<"assign"<<std::endl;
        live::INodePtr n = selectStack->PopAndRemove();
        std::set<std::string*> okColors;
        std::list<temp::Temp *> reg_list=reg_manager->Registers()->GetList();
        temp::Temp * rsp=reg_manager->StackPointer();
        for(const auto reg:reg_list)
        {
            if(reg==rsp) continue;
            std::string* key=temp::Map::Name()->Look(reg);
            okColors.insert(key);
        }
        // remove the neighbour color in OKcolor
        std::list<live::INodePtr> w_list = n->Adj()->GetList();

        std::cout<<"cur node:"<<n->NodeInfo()->Int()<<std::endl;
        std::cout<<"neighbour size:"<<w_list.size()<<std::endl;

        for(const auto w:w_list)
        {
            if(coloredNodes->Contain(GetAlias(w))
                ||precolored->count(GetAlias(w)->NodeInfo()))
            {
                auto test=(*color)[GetAlias(w)];
                std::cout<<"neighbour used:"<<*test<<std::endl;
                okColors.erase((*color)[GetAlias(w)]);
            }
        }
        if(okColors.empty())
        {
            std::cout<<"okColor empty spill:"<<n->NodeInfo()->Int()<<std::endl;
            spilledNodes->Append(n);
        }
        else
        {
            coloredNodes->Append(n);
            auto c=okColors.begin();
            (*color)[n]=(*c);
            std::cout<<"temp:"<<n->NodeInfo()->Int()<<" color:"<<*(*c)<<std::endl;
        }
    }
    std::list<live::INodePtr> n_list = coalescedNodes->GetList();
    for(const auto n:n_list)
    {
        std::cout<<"coalesce temp:"<<n->NodeInfo()->Int()<<" color:"<<*(*color)[GetAlias(n)]<<std::endl;
        (*color)[n]=(*color)[GetAlias(n)];
    }
}

// live::INodeListPtr RegAllocator::SetToList(std::unordered_map<live::INodePtr,std::string*>* set_)
// {
//     live::INodeListPtr ptr=new live::INodeList();
//     for(auto it:*set_)
//     {
//         ptr->Append(it.first);
//     }
//     return ptr;
// }

void RegAllocator::RewriteProgram()
{
    // todo: delete instr directly in graph
    std::cout<<"rewrite program"<<std::endl;
    noSpillingTemp->clear();
    std::list<live::INodePtr> spill_list = spilledNodes->GetList();
    std::cout<<"spill list size:"<<spill_list.size()<<std::endl;
    std::vector<std::pair<assem::Instr*,assem::Instr*>> mp;
    for(auto it=spill_list.begin();it!=spill_list.end();++it)
    {
        live::INodePtr node=*it;
        temp::Temp * spilled_temp = node->NodeInfo();
        std::list<assem::Instr *> instrs_list = assem_instr->GetInstrList()->GetList();
        frame->offset-=reg_manager->WordSize();
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
                // read: insert before
                std::string instr_raw="movq ("+frame->name->Name()+"_framesize-"+std::to_string(-frame->offset)+")(`s0), `d0";
                mp.push_back(std::make_pair(instr,new assem::OperInstr(instr_raw,new temp::TempList(new_temp),new temp::TempList(reg_manager->StackPointer()),nullptr)));
            }

            if(temp_dst&&Contain(temp_dst,spilled_temp))
            {
                temp::Temp* new_temp = temp::TempFactory::NewTemp();
                noSpillingTemp->emplace(new_temp);
                for(auto& t:temp_dst->GetRawList())
                {
                    if(t==spilled_temp)
                    {
                        t=new_temp;
                        break;
                    }
                }
                // store:insert after
                std::string instr_raw="movq `s0, ("+frame->name->Name()+"_framesize-"+std::to_string(-frame->offset)+")(`d0)";
                mp.push_back(std::make_pair(instr,new assem::OperInstr(instr_raw,new temp::TempList(reg_manager->StackPointer()),new temp::TempList(new_temp),nullptr)));

            }

        }
    }
    for(const auto pr:mp)
    {
        auto it=assem_instr->GetInstrList()->GetList().begin();
        for(;it!=assem_instr->GetInstrList()->GetList().end();++it)
        {
            if(*it==pr.first)
            {
                // if read, insert before current instr
                if(((assem::OperInstr*)pr.second)->assem_.find("movq (")!=std::string::npos)
                {
                    assem_instr->GetInstrList()->Insert(it,pr.second);
                }
                else
                {
                    auto next=it;
                    next++;
                    assem_instr->GetInstrList()->Insert(next,pr.second);
                }
                break;
            }
        }
    }
    ShowInstrList(assem_instr->GetInstrList());
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

// temp::Map* RegAllocator::ToMap(std::unordered_map<live::INodePtr,std::string*>* color)
// {
//     temp::Map* color_map=temp::Map::Empty();
//     auto origin_map=*color;
//     for(const auto item:origin_map)
//     {
//         color_map->Enter(item.first->NodeInfo(),item.second);
//     }
//     return color_map;
// }

} // namespace ra