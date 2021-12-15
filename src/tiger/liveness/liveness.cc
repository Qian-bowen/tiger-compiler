#include "tiger/liveness/liveness.h"
#include <unordered_set>

extern frame::RegManager *reg_manager;

namespace live {

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
  for (auto move : list->GetList()) {
    if (!Contain(move.first, move.second))
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

bool IsTempListEqual(const temp::TempList* a, const temp::TempList* b)
{
  std::list<temp::Temp *> a_list = a->GetList();
  std::list<temp::Temp *> b_list = b->GetList();
  if(a_list.size()!=b_list.size()) return false;
  std::unordered_set<temp::Temp *> a_set;
  for(auto a_temp:a_list)
  {
    a_set.insert(a_temp);
  }
  for(auto b_temp:b_list)
  {
    if(!a_set.count(b_temp)) return false;
  }
  return true;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */

  while(true)
  {
    graph::Table<assem::Instr, temp::TempList> in_prev_table =*(in_.get());
    graph::Table<assem::Instr, temp::TempList> out_prev_table =*(out_.get());

    const std::list<fg::FNodePtr> nodes = flowgraph_->Nodes()->GetList();
    for(const auto& node:nodes)
    {
      temp::TempList* in_cur = in_->Look(node);
      temp::TempList* out_cur = out_->Look(node);
      temp::TempList* in_to_up = in_cur;
      temp::TempList* out_to_up = out_cur;
      assem::Instr* info = node->NodeInfo();

      // update new in of n
      temp::TempList* in_new = new temp::TempList();
      std::list<temp::Temp *> use_n = info->Use()->GetList();
      std::list<temp::Temp *> def_n = info->Def()->GetList();
      std::list<temp::Temp *> out_n = out_cur->GetList();
      for(auto use:use_n)
      {
        in_new->Append(use);
      }
      std::unordered_set<temp::Temp *> def_set;
      for(auto def:def_n)
      {
        def_set.insert(def);
      }
      for(auto out:out_n)
      {
        if(!def_set.count(out))
        {
          in_new->Append(out);
        }
      }
      // update in
      in_->Set(node,in_new);

      //update out
      temp::TempList* out_new = new temp::TempList();
      std::list<fg::FNodePtr> node_succs = node->Succ()->GetList();
      for(auto succ:node_succs)
      {
        std::list<temp::Temp *> in_of_succ = in_->Look(succ)->GetList();
        for(auto in:in_of_succ)
        {
          out_new->Append(in);
        }
      }

      //update out
      out_->Enter(node,out_new);
    }

    // test and break
    bool all_equal=true;

    for(const auto& node:nodes)
    {
      const temp::TempList* prev_in = in_prev_table.Look(node);
      const temp::TempList* prev_out = out_prev_table.Look(node);
      const temp::TempList* cur_in = in_->Look(node);
      const temp::TempList* cur_out = out_->Look(node);
      if(!(IsTempListEqual(prev_in,cur_in)
        &&IsTempListEqual(prev_out,cur_out)))
      {
        all_equal=false;
        break;
      }
    }

    if(all_equal) break;
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  std::list<fg::FNodePtr> flow_nodes = flowgraph_->Nodes()->GetList();
  for(const auto flow_node:flow_nodes)
  {
    assem::Instr* instr = flow_node->NodeInfo();
    std::list<temp::Temp *> instr_use = instr->Use()->GetList();
    std::list<temp::Temp *> instr_def = instr->Def()->GetList();
    std::list<temp::Temp *> out_active_list = out_->Look(flow_node)->GetList();

    if(typeid(*instr)==typeid(assem::MoveInstr))
    {
      std::unordered_set<temp::Temp *> use_set;
      for(const auto use_temp:instr_use)
      {
        use_set.insert(use_temp);
      }
      for(const auto out_active:out_active_list)
      {
        if(!use_set.count(out_active))
        {
          assert(instr_def.size()==1);
          INodePtr to_node=GetOrCreateNode(instr_def.front());
          INodePtr from_node=GetOrCreateNode(out_active);
          live_graph_.interf_graph->AddEdge(from_node,to_node);
          live_graph_.moves->Append(from_node,to_node);
        }
      }
    }
    else if(typeid(*instr)==typeid(assem::OperInstr))
    {
      for(const auto out_active:out_active_list)
      {
        assert(instr_def.size()==1);
        INodePtr to_node=GetOrCreateNode(instr_def.front());
        INodePtr from_node=GetOrCreateNode(out_active);
        live_graph_.interf_graph->AddEdge(from_node,to_node);
        live_graph_.moves->Append(from_node,to_node);
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

INodePtr LiveGraphFactory::GetOrCreateNode(temp::Temp* temp)
{
  INodePtr node = temp_node_map_->Look(temp);
  if(node!=nullptr) return node;
  INodePtr new_node = live_graph_.interf_graph->NewNode(temp);
  temp_node_map_->Enter(temp,new_node);
  return new_node;
}

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

} // namespace live