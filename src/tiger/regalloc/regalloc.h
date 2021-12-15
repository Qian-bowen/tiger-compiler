#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

#include <set>
#include <unordered_set>
#include <stack>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result();
};

class RegAllocator {
  /* TODO: Put your lab6 code here */

  frame::Frame* frame;
  std::unique_ptr<cg::AssemInstr> assem_instr;

  fg::FlowGraphFactory* flow_graph_factory;
  live::LiveGraphFactory* live_graph_factory;

  live::INodeListPtr simplifyWorklist;
  live::INodeListPtr freezeWorklist;
  live::INodeListPtr spillWorklist;

  live::INodeListPtr selectStack;

  live::MoveList* coalescedMoves;
  live::MoveList* constrainedMoves;
  live::MoveList* frozenMoves;
  live::MoveList* worklistMoves;
  live::MoveList* activeMoves;

  live::INodeListPtr spilledNodes;
  live::INodeListPtr coalescedNodes;
  live::INodeListPtr coloredNodes;

  std::unordered_map<live::INodePtr,int>* degree;
  std::unordered_map<live::INodePtr,live::MoveList*>* moveList;
  std::unordered_map<live::INodePtr,live::INodePtr>* alias;
  std::unordered_map<live::INodePtr,std::string*>* color;

  
  std::set<live::INodePtr>* precolored;
  std::set<temp::Temp*>* noSpillingTemp;

  Result* result;

  void Main(frame::Frame* f,assem::InstrList* instrs);

  void Build();
  void MakeWorklist();

  void Simplify();
  void DecrementDegree(live::INodePtr m);
  void EnableMoves(live::INodeListPtr nodes);
  live::INodeListPtr Adjacent(live::INodePtr n);
  live::MoveList* NodeMoves(live::INodePtr n);
  bool MoveRelated(live::INodePtr n);

  void Coalesce();
  void AddWorkList(live::INodePtr n);
  live::INodePtr GetAlias(live::INodePtr n);
  bool OK(live::INodePtr t,live::INodePtr r);
  void Combine(live::INodePtr u,live::INodePtr v);
  void AddEdge(live::INodePtr u,live::INodePtr v);
  bool Conservative(live::INodeListPtr node_list);

  void Freeze();
  void FreezeMoves(live::INodePtr u);

  void SelectSpill();

  void AssignColors();
  void RewriteProgram(frame::Frame* f,assem::InstrList* instrs);

  //util function
  live::INodeListPtr SetToList(std::set<live::INodePtr>* set_);
  bool Contain(temp::TempList* temp_list,temp::Temp* temp);
  temp::Map* ToMap(std::unordered_map<live::INodePtr,std::string*>* color);


public:
  explicit RegAllocator(frame::Frame* frame_,std::unique_ptr<cg::AssemInstr> assem_instr_);
  void RegAlloc(){Main(frame,assem_instr.get()->GetInstrList());}
  std::unique_ptr<ra::Result> TransferResult(){return std::unique_ptr<ra::Result>(result);}

};


} // namespace ra

#endif