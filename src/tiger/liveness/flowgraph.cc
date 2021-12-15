#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  std::list<assem::Instr *> instrs = instr_list_->GetList();
  FNodePtr prev_node = nullptr;
  for(const auto instr:instrs)
  {
    FNodePtr node = flowgraph_->NewNode(instr);

    if(prev_node){
      flowgraph_->AddEdge(prev_node,node);
    }

    if(typeid(*instr)==typeid(assem::LabelInstr)){
      assem::LabelInstr* label_instr=static_cast<assem::LabelInstr*>(instr);
      label_map_->Enter(label_instr->label_,node);
    }

    if((typeid(*instr)==typeid(assem::OperInstr))
      &&(((assem::OperInstr*)instr)->assem_.find("jmp")==0)){
      assem::OperInstr* oper_instr=static_cast<assem::OperInstr*>(instr);
      // not set prev if cur is jump
      prev_node=nullptr;
    }else{
      prev_node=node;
    }
  }

  auto node_list=flowgraph_->Nodes()->GetList();
  for(const auto node:node_list)
  {
    assem::Instr * instr = node->NodeInfo();
    if((typeid(*instr)==typeid(assem::OperInstr))
      && (((assem::OperInstr*)instr)->jumps_))
    {
      assem::Targets * targets = ((assem::OperInstr*)instr)->jumps_;
      assert(targets!=nullptr);
      if(targets->labels_!=nullptr)
      {
        std::vector<temp::Label *> labels = *(targets->labels_);
        assert(0!=labels.size());
        for(const auto label:labels)
        {
          flowgraph_->AddEdge(node,label_map_->Look(label));
        }
      }
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
