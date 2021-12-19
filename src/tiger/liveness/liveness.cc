#include "tiger/liveness/liveness.h"
#include <unordered_map>
#include <set>

extern frame::RegManager *reg_manager;

namespace live {

void ShowInstrInfo(assem::Instr* instr)
{
  std::cout<<"******instr info******"<<std::endl;
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
  std::cout<<"**********************"<<std::endl;
}

void ShowLivenessGraph(LiveGraph live_graph)
{
  std::cout<<"--------------live graph---------------"<<std::endl;
  std::list<live::INodePtr> nodes = live_graph.interf_graph->Nodes()->GetList();
  for(const auto node:nodes)
  {
    std::cout<<node->NodeInfo()->Int()<<std::endl;
    std::cout<<"succ:";
    for(const auto succ:node->Succ()->GetList())
    {
      std::cout<<succ->NodeInfo()->Int()<<" ";
    }
    std::cout<<std::endl;
    std::cout<<"pred:";
    for(const auto pred:node->Pred()->GetList())
    {
      std::cout<<pred->NodeInfo()->Int()<<" ";
    }
    std::cout<<std::endl;
  }
  std::cout<<"---------------------------------------"<<std::endl;
}

void ShowInOutInfo(fg::FGraphPtr flowgraph_,std::unordered_map<assem::Instr*, temp::TempList*>& in,std::unordered_map<assem::Instr*, temp::TempList*>& out)
{
  std::cout<<"--------------in out info---------------"<<std::endl;
  for(const auto node:flowgraph_->Nodes()->GetList())
  {
    ShowInstrInfo(node->NodeInfo());
    std::list<temp::Temp *> ins = in[node->NodeInfo()]->GetList();
    std::cout<<"in:";
    for(const auto i:ins)
    {
      std::cout<<i->Int()<<" ";
    }
    std::cout<<std::endl;

    std::list<temp::Temp *> outs = out[node->NodeInfo()]->GetList();
    std::cout<<"outs:";
    for(const auto i:outs)
    {
      std::cout<<i->Int()<<" ";
    }
    std::cout<<std::endl;

    std::cout<<"use:";
    for(const auto i:node->NodeInfo()->Use()->GetList())
    {
      std::cout<<i->Int()<<" ";
    }
    std::cout<<std::endl;

    std::cout<<"def:";
    for(const auto i:node->NodeInfo()->Def()->GetList())
    {
      std::cout<<i->Int()<<" ";
    }
    std::cout<<std::endl;
  }
  std::cout<<"----------------------------------------"<<std::endl;
}

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}


bool IsEqualTempList(temp::TempList* left,temp::TempList* right)
{
  std::set<int> lset,rset;
  std::list<temp::Temp *> l_list = left->GetList();
  std::list<temp::Temp *> r_list = right->GetList();
  for(const auto l:l_list)
  {
    lset.insert(l->Int());
  }
  for(const auto r:r_list)
  {
    rset.insert(r->Int());
  }
  return lset==rset;
}

bool IsEqual(std::unordered_map<assem::Instr*, temp::TempList*>& a_map, std::unordered_map<assem::Instr*, temp::TempList*>& b_map)
{
  if(a_map.size()!=b_map.size()) return false;
  for(const auto a:a_map)
  {
    if(b_map.find(a.first)==b_map.end()) return false;
    if(!IsEqualTempList(a.second,b_map[a.first])) return false;
  }
  return true;
}

bool Contain_TL(temp::TempList* temps,temp::Temp* temp)
{
  for(const auto t:temps->GetList())
  {
    if(t==temp) return true;
  }
  return false;
}

temp::TempList* Union_TL(temp::TempList* left,temp::TempList* right)
{
  temp::TempList* res=new temp::TempList();
  auto left_list=left->GetList();
  auto right_list=right->GetList();
  for(const auto t:left_list)
  {
    if(!Contain_TL(res,t)) res->Append(t);
  }
  for(const auto t:right_list)
  {
    if(!Contain_TL(res,t)) res->Append(t);
  }
  return res;
}

temp::TempList* Diff_TL(temp::TempList* left,temp::TempList* right)
{
  temp::TempList* res=new temp::TempList();
  auto left_list=left->GetList();
  for(const auto t:left_list)
  {
    if(!Contain_TL(right,t)) res->Append(t);
  }
  return res;
}


void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  in_.clear();
  out_.clear();
  std::list<fg::FNodePtr> flow_nodes = flowgraph_->Nodes()->GetList();
  for(const auto& node:flow_nodes)
  {
    in_[node->NodeInfo()]=new temp::TempList();;
    out_[node->NodeInfo()]=new temp::TempList();;
  }
  

  while(true)
  {
    std::cout<<"loop live map"<<std::endl;
    std::unordered_map<assem::Instr*, temp::TempList*> in_prev_table =in_;
    std::unordered_map<assem::Instr*, temp::TempList*> out_prev_table =out_;

    const std::list<fg::FNodePtr> nodes_raw = flowgraph_->Nodes()->GetList();

    for(const auto& node:nodes_raw)
    {
      assem::Instr* info = node->NodeInfo();

      auto use_n = info->Use();
      auto def_n = info->Def();
    
      // update in
      in_[info]=Union_TL(use_n,Diff_TL(out_[info],def_n));

      //update out
      std::list<fg::FNodePtr> node_succs = node->Succ()->GetList();
      // std::cout<<"node:"<<std::string(typeid(*(node->NodeInfo())).name())<<" succ size:"<<node_succs.size()<<std::endl;
      // std::cout<<"out before size:"<<out_[info]->GetList().size()<<std::endl;
      // ShowInstrInfo(info);
      // std::cout<<"succ info"<<std::endl;
      for(auto succ:node_succs)
      {
        // ShowInstrInfo(succ->NodeInfo());
        // std::cout<<"union left size:"<<out_[info]->GetList().size()<<"  right size:"<<in_[succ->NodeInfo()]->GetList().size()<<std::endl;
        out_[info]=Union_TL(out_[info],in_[succ->NodeInfo()]);
      }
      // std::cout<<"out after size:"<<out_[info]->GetList().size()<<std::endl;
    }

    // test and break

    if(IsEqual(in_,in_prev_table)
      &&IsEqual(out_,out_prev_table))
    {
      break;
    }
  }
  ShowInOutInfo(flowgraph_,in_,out_);
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */

  // precolor
  std::list<temp::Temp *> reg_list = reg_manager->Registers()->GetList(); // no rsp
  for(const auto reg:reg_list)
  {
    live::INodePtr node = live_graph_.interf_graph->NewNode(reg);
    temp_node_map_->Enter(node->NodeInfo(),node);
  }

  for(auto reg1:reg_list)
  {
    for(auto reg2:reg_list)
    {
      if(reg1==reg2) continue;
      live_graph_.interf_graph->AddEdge(temp_node_map_->Look(reg1),temp_node_map_->Look(reg2));
    }
  }

  std::cout<<"interfgraph begin"<<std::endl;
  // create nodes in instruction
  std::list<fg::FNodePtr> flow_nodes = flowgraph_->Nodes()->GetList();
  // reverse instrs
  // enter node
  for(auto node:flow_nodes)
  {
    const std::list<temp::Temp *> use_nodes = node->NodeInfo()->Use()->GetList();
    const std::list<temp::Temp *> def_nodes = node->NodeInfo()->Def()->GetList();
    for(auto n:use_nodes)
    {
      if(n==reg_manager->StackPointer()) continue;
      if(!temp_node_map_->Look(n))
      {
        live::INodePtr new_node = live_graph_.interf_graph->NewNode(n);
        temp_node_map_->Enter(n,new_node);
      }
    }
    for(auto n:def_nodes)
    {
      if(n==reg_manager->StackPointer()) continue;
      if(!temp_node_map_->Look(n))
      {
        live::INodePtr new_node = live_graph_.interf_graph->NewNode(n);
        temp_node_map_->Enter(n,new_node);
      }
    }
  }

  // std::list<fg::FNodePtr> flow_nodes = flowgraph_->Nodes()->GetList();
  for(const auto flow_node:flow_nodes)
  {
    assem::Instr* instr = flow_node->NodeInfo();
    std::list<temp::Temp *> instr_use = instr->Use()->GetList();
    std::list<temp::Temp *> instr_def = instr->Def()->GetList();

    // if dst or src is null or empty, means no data move
    if(typeid(*instr)==typeid(assem::MoveInstr))
    {
      ShowInstrInfo(instr);
      for(const auto def:instr_def)
      {
        std::list<temp::Temp *> out_active_list = out_[flow_node->NodeInfo()]->GetList();
        // std::cout<<"def:"<<def->Int()<<" out active size:"<<out_active_list.size()<<std::endl;
        for(const auto out_active:out_active_list)
        {
          // std::cout<<out_active->Int()<<" ";
          if(IsContain(instr_use,out_active)) continue;
          if((def==reg_manager->StackPointer())
            ||(out_active==reg_manager->StackPointer()))
          {continue;}
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def),temp_node_map_->Look(out_active));
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out_active),temp_node_map_->Look(def));
          // std::cout<<"add edge:"<<temp_node_map_->Look(def)->NodeInfo()->Int()<<" "<<temp_node_map_->Look(out_active)->NodeInfo()->Int()<<std::endl;
        }
        // std::cout<<std::endl;

        // add movelist
        for(const auto use:instr_use)
        {
          if((def==reg_manager->StackPointer())
            ||(use==reg_manager->StackPointer()))
          {continue;}
          if(!live_graph_.moves->Contain(temp_node_map_->Look(use),temp_node_map_->Look(def)))
          {
            std::cout<<"add move"<<std::endl;
            live_graph_.moves->Append(temp_node_map_->Look(use),temp_node_map_->Look(def));
          }
        }
        
      }
    }
    else if(typeid(*instr)==typeid(assem::OperInstr))
    {
      for(const auto def:instr_def)
      {
        std::list<temp::Temp *> out_active_list = out_[flow_node->NodeInfo()]->GetList();
        // std::cout<<"def:"<<def->Int()<<" out active size:"<<out_active_list.size()<<std::endl;
        for(const auto out_active:out_active_list)
        {
          // std::cout<<out_active->Int()<<" ";
          if((def==reg_manager->StackPointer())
            ||(out_active==reg_manager->StackPointer()))
          {continue;}
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def),temp_node_map_->Look(out_active));
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out_active),temp_node_map_->Look(def));
          // std::cout<<"add edge:"<<temp_node_map_->Look(def)->NodeInfo()->Int()<<" "<<temp_node_map_->Look(out_active)->NodeInfo()->Int()<<std::endl;
        }
        // std::cout<<std::endl;

      }
    }
  }
  ShowLivenessGraph(live_graph_);
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

// INodePtr LiveGraphFactory::GetOrCreateNode(temp::Temp* temp)
// {
//   INodePtr node = temp_node_map_->Look(temp);
//   if(node!=nullptr) return node;
//   INodePtr new_node = live_graph_.interf_graph->NewNode(temp);
//   temp_node_map_->Enter(temp,new_node);
//   return new_node;
// }

MoveList* MoveList::GetRelatedMove(INodePtr node_ptr)
{
  MoveList* ml=new MoveList();
  for(const auto move:move_list_)
  {
    if(move.first==node_ptr||move.second==node_ptr)
    {
      ml->Append(move.first,move.second);
    }
  }
  return ml;
}

bool LiveGraphFactory::IsContain(std::list<temp::Temp *>& container,temp::Temp * target)
{
  for(const auto item:container)
  {
    if(target==item) return true;
  }
  return false;
}

} // namespace live
