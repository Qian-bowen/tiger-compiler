#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  // set framesize label
  // std::cout<<"frame_:"<<this->frame_<<std::endl;
  fs_=this->frame_->name->Name()+"_framesize";
  // std::cout<<"fs:"<<fs_<<std::endl;

  std::list<temp::Temp *> callee_regs = reg_manager->CalleeSaves()->GetList();
  std::vector<temp::Temp *> protect_regs_list;

  // init instr list
  assem::InstrList* instr_list=new assem::InstrList();
  assem_instr_=std::make_unique<AssemInstr>(instr_list);

  // protect
  // move the callee save regs value to other regs, for its callee's responsibility to protect them
  for(const auto& reg:callee_regs)
  {
    temp::Temp* to_reg=temp::TempFactory::NewTemp();
    protect_regs_list.push_back(to_reg);
    assem_instr_->GetInstrList()->Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList({to_reg}),
        new temp::TempList({reg})));
  }

  // munch
  std::list<tree::Stm *> stm_list = traces_->GetStmList()->GetList();
  for(const auto& stm:stm_list)
  {
    stm->Munch(*(assem_instr_->GetInstrList()),fs_);
  }


  // restore value to callee save rigister
  assert(callee_regs.size()==protect_regs_list.size());
  int count=0;
  for(const auto& reg:callee_regs)
  {
    assem_instr_->GetInstrList()->Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList({reg}),
        new temp::TempList({protect_regs_list[count]})));
    count++;
  }

  // program exit
  // rsp,rax, and all callee save reigsters still active, can not be used for other purpose
  assem_instr_->GetInstrList()->Append(
    new assem::OperInstr("",nullptr,reg_manager->ReturnSink(),new assem::Targets(nullptr)));

}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list,fs);
  this->right_->Munch(instr_list,fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // std::cout<<"label stm:"<<label_<<std::endl;
  // std::cout<<"label name:"<<label_->Name()<<std::endl;
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(label_),label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Label* jump_label=exp_->name_;
  std::string instr_str="jmp "+temp::LabelFactory::LabelString(jump_label);
  instr_list.Append(new assem::OperInstr(instr_str,nullptr,nullptr,new assem::Targets(jumps_)));

}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* left = left_->Munch(instr_list,fs);
  temp::Temp* right = right_->Munch(instr_list,fs);
  // set condition code based on s1-s0
  // cmpq right,left
  instr_list.Append(new assem::OperInstr("cmpq `s0, `s1",nullptr,new temp::TempList({right,left}),nullptr));
  std::string instr_str="";
  switch(this->op_)
  {
    case tree::RelOp::EQ_OP: instr_str+="je"; break;
    case tree::RelOp::NE_OP: instr_str+="jne"; break;
    case tree::RelOp::LT_OP: instr_str+="jl"; break;
    case tree::RelOp::GT_OP: instr_str+="jg"; break;
    case tree::RelOp::LE_OP: instr_str+="jle"; break;
    case tree::RelOp::GE_OP: instr_str+="jge"; break;
  }
  instr_str+=" `j0";
  instr_list.Append(new assem::OperInstr(instr_str,nullptr,nullptr,new assem::Targets(new std::vector<temp::Label *>{this->true_label_})));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // if src is mem, means read from mem, so just munch it with MemExp, get the value from returned register
  // mem on dst, means store, memExp just means read mem value to register
  assert(typeid(*src_)==typeid(tree::TempExp));
  if(typeid(*dst_)==typeid(tree::MemExp))
  {
    temp::Temp* src = src_->Munch(instr_list,fs);
    tree::MemExp* dst = static_cast<tree::MemExp*>(dst_);
    // get the register of dst exp directly
    temp::Temp* dst_exp_reg = dst->exp_->Munch(instr_list,fs);
    instr_list.Append(new assem::OperInstr("movq `s0, (`s1)",nullptr,new temp::TempList({src,dst_exp_reg}),nullptr));
    return;
  }
  else if(typeid(*dst_)==typeid(tree::TempExp))
  {
    temp::Temp* src = src_->Munch(instr_list,fs);
    temp::Temp* dst = dst_->Munch(instr_list,fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({dst}),new temp::TempList({src})));
    return;
  }
  assert(0);
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list,fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* reg = temp::TempFactory::NewTemp();
  temp::Temp* left = left_->Munch(instr_list,fs);
  temp::Temp* right = right_->Munch(instr_list,fs);
  switch(this->op_)
  {
    case tree::PLUS_OP:
    {
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({reg}),new temp::TempList({left})));
      // reg is source and destination, its value will be read and written. though reg not appear in source
      instr_list.Append(new assem::OperInstr("addq `s0, `d0",new temp::TempList({reg}),new temp::TempList({right,reg}),nullptr));
      break;
    }
    case tree::MINUS_OP:
    {
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({reg}),new temp::TempList({left})));
      instr_list.Append(new assem::OperInstr("subq `s0, `d0",new temp::TempList({reg}),new temp::TempList({right,reg}),nullptr));
      break;
    }
    case tree::MUL_OP:
    {
      temp::Temp* rdx_spec=reg_manager->Registers()->NthTemp(3);
      temp::Temp* rax_spec=reg_manager->ReturnValue();
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({rax_spec}),new temp::TempList({left})));
      // extend rax to rax and rdx
      instr_list.Append(new assem::OperInstr("cqto", new temp::TempList({rax_spec,rdx_spec}), new temp::TempList({rax_spec}), nullptr));
      instr_list.Append(new assem::OperInstr("imulq `s0",new temp::TempList({rax_spec,rdx_spec}),new temp::TempList({right,rax_spec,rdx_spec}),nullptr));
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({reg}),new temp::TempList({rax_spec})));
      break;
    }
    case tree::DIV_OP:
    {
      temp::Temp* rdx_spec=reg_manager->Registers()->NthTemp(3);
      temp::Temp* rax_spec=reg_manager->ReturnValue();
      // divide is special in x86
      // move dividend to rax first
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({rax_spec}),new temp::TempList({left})));
      // rax and rdx act as a oct word dividend, right acts as divisor
      instr_list.Append(new assem::OperInstr("cqto", new temp::TempList({rax_spec,rdx_spec}), new temp::TempList({rax_spec}), nullptr));
      instr_list.Append(new assem::OperInstr("idivq `s0",new temp::TempList({rax_spec,rdx_spec}),new temp::TempList({right, rax_spec,rdx_spec}),nullptr));
      // move the rax which stores smaller bytes to result register
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({reg}),new temp::TempList({rax_spec})));
      break;
    }
    default:assert(0);
  }
  return reg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // the result of memory operation store to another register
  temp::Temp* reg = temp::TempFactory::NewTemp();
  temp::Temp* exp_reg = exp_->Munch(instr_list,fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0",new temp::TempList({reg}),new temp::TempList({exp_reg}),nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // if virtual frame pointer, convert to stack pointer + frame size
  if(temp_==reg_manager->FramePointer())
  {
    // framesize is set to specific size in .set
    // .set name, expr assign name with expression
    // so it will be replaced with a value when linking
    temp::Temp* reg = temp::TempFactory::NewTemp();
    std::string instr_str="leaq "+std::string(fs)+"(`s0), `d0";
    instr_list.Append(new assem::OperInstr(instr_str,new temp::TempList({reg}),new temp::TempList({reg_manager->StackPointer()}),nullptr));
    return reg;
  }
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list,fs);
  return exp_->Munch(instr_list,fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // RIP is the instruction pointer register, find address in relative way
  // name dose not means absolute address. Points to the name in RIP relative way
  // key reason is the offset between text sector and data sector is fixed
  std::string instr_str="leaq "+this->name_->Name()+"(%rip), `d0";
  temp::Temp* name_addr = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::OperInstr(instr_str,new temp::TempList({name_addr}),nullptr,nullptr));
  return name_addr;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* reg = temp::TempFactory::NewTemp();
  std::string instr_str="movq $"+std::to_string(consti_)+", `d0";
  instr_list.Append(new assem::OperInstr(instr_str,new temp::TempList({reg}),nullptr,nullptr));
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  assert(typeid(*fun_)==typeid(tree::NameExp));
  temp::Temp* ret_reg=temp::TempFactory::NewTemp();
  tree::NameExp* func_name=static_cast<tree::NameExp*>(fun_);

  // before munch args, protect the registers that convey args values
  // std::list<temp::Temp *> arg_regs = reg_manager->ArgRegs()->GetList();
  // std::list<temp::Temp *> protect_regs_list;
  // int args_num=this->args_->GetList().size(), arg_cnt=0;
  // for(const auto& reg:arg_regs)
  // {
  //   if(arg_cnt>=args_num) break;
  //   temp::Temp* to_reg=temp::TempFactory::NewTemp();
  //   protect_regs_list.push_back(to_reg);
  //   instr_list.Append(
  //     new assem::MoveInstr(
  //       "movq `s0, `d0",
  //       new temp::TempList({to_reg}),
  //       new temp::TempList({reg})));
  //   ++arg_cnt;
  // }

  // handle all args correctly before call
  // used_temp is for liveness analysis later
  temp::TempList* used_temp = this->args_->MunchArgs(instr_list,fs);
  std::string instr_str="call "+func_name->name_->Name();
  
  // calldefs only need caller save registers
  // return address is in %rip, which is never allocated for other purpose. return value register is protected in next movq instruction

  // use direct call, namely call label directly
  instr_list.Append(new assem::OperInstr(instr_str,reg_manager->CallerSaves(),used_temp,nullptr));
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({ret_reg}),new temp::TempList({reg_manager->ReturnValue()})));
  // add rsp if push more args on stack

  int used_num = used_temp->GetList().size();
  int args_reg_num=reg_manager->ArgRegs()->GetList().size();
  if(used_num>args_reg_num)
  {
    std::string add_rsp_instr="addq $"+std::to_string((used_num-args_reg_num)*reg_manager->WordSize())+", `d0";
    instr_list.Append(new assem::OperInstr(add_rsp_instr,new temp::TempList({reg_manager->StackPointer()}),nullptr,nullptr));
  }

  return ret_reg;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::list<tree::Exp *> args_list = GetList();
  std::list<temp::Temp *> tem_list = reg_manager->ArgRegs()->GetList();
  temp::TempList* used_temp=new temp::TempList();
  int tem_reg_max=tem_list.size(),cnt=0;
  temp::Temp* pos_reg=temp::TempFactory::NewTemp();
  for(const auto& arg:args_list)
  {
    temp::Temp* tem = arg->Munch(instr_list,fs);
    used_temp->Append(tem);
    if(cnt<tem_reg_max)
    {
      // move result of parameter to args register
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
        new temp::TempList({reg_manager->ArgRegs()->NthTemp(cnt)}),
        new temp::TempList({tem})));
    }
    else
    {
      // push the value to stack
      // no push in lab5, so just sub and move
      // std::string rsp_copy_str="movq %rsp, `d0";
      // instr_list.Append(new assem::OperInstr(rsp_copy_str,new temp::TempList({pos_reg}),nullptr,nullptr));
      // std::string sub_str=std::to_string((cnt-6)*reg_manager->WordSize());
      // std::string rsp_copy_sub_str="subq $8, `d0";
      // instr_list.Append(new assem::OperInstr(rsp_copy_sub_str,new temp::TempList({pos_reg}),nullptr,nullptr));
      // std::string instr_str="movq `s0, (`s1)";
      // instr_list.Append(new assem::OperInstr(instr_str,new temp::TempList({pos_reg}),new temp::TempList({tem}),nullptr));
      // temp::Temp* pos_reg=temp::TempFactory::NewTemp();

      std::string sub_rsp="subq $8, %rsp";
      instr_list.Append(new assem::OperInstr(sub_rsp,new temp::TempList({reg_manager->StackPointer()}),nullptr,nullptr));
      std::string instr_str="movq `s0, (%rsp)";
      instr_list.Append(new assem::OperInstr(instr_str,new temp::TempList({reg_manager->StackPointer()}),new temp::TempList({tem}),nullptr));
    }
    ++cnt;
  }
  return used_temp;
}

} // namespace tree
