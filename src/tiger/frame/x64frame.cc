#include "tiger/frame/x64frame.h"
#include <assert.h>

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;
/* TODO: Put your lab5 code here */
  explicit InFrameAccess(int offset) : offset(offset) {}
  virtual tree::Exp* ToExp(tree::Exp* framePtr)const override;
  virtual ~InFrameAccess()=default;
  
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;
/* TODO: Put your lab5 code here */
  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  virtual tree::Exp* ToExp(tree::Exp* framePtr)const override;
  virtual ~InRegAccess()=default;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  int arg_reg_used=1; // first is static link by default
  X64Frame():Frame(){};
  virtual ~X64Frame()=default;
  virtual Access* AllocLocal(bool is_local)override;
  virtual std::string GetLabel()override{return this->name->Name();}
};
/* TODO: Put your lab5 code here */

tree::Exp* InFrameAccess::ToExp(tree::Exp* framePtr)const
{
  return new tree::MemExp(
    new tree::BinopExp(tree::PLUS_OP,framePtr,new tree::ConstExp(offset)));
}

tree::Exp* InRegAccess::ToExp(tree::Exp* framePtr)const
{
  return new tree::TempExp(reg);
}

tree::Exp* exp(frame::Access* access,tree::Exp* framePtr)
{
  return access->ToExp(framePtr);
}

Access* X64Frame::AllocLocal(bool is_local)
{
  frame::Access* acc=nullptr;
  int args_reg_max=reg_manager->ArgRegs()->GetList().size();
  if((!is_local)&&(arg_reg_used<args_reg_max))
  {
    acc=new frame::InRegAccess(temp::TempFactory::NewTemp());
    ++arg_reg_used;
  }
  else{
    acc=new frame::InFrameAccess(offset);
    offset-=reg_manager->WordSize(); // attention: large address is upper
  }
  return acc;
}


X64RegManager::X64RegManager()
{
  // std::cout<<"reg init begin"<<std::endl;
  rax=temp::TempFactory::NewTemp();
  rbx=temp::TempFactory::NewTemp();
  rcx=temp::TempFactory::NewTemp();
  rdx=temp::TempFactory::NewTemp();
  rsi=temp::TempFactory::NewTemp();
  rdi=temp::TempFactory::NewTemp();
  rbp=temp::TempFactory::NewTemp();
  rsp=temp::TempFactory::NewTemp();
  r8=temp::TempFactory::NewTemp();
  r9=temp::TempFactory::NewTemp();
  r10=temp::TempFactory::NewTemp();
  r11=temp::TempFactory::NewTemp();
  r12=temp::TempFactory::NewTemp();
  r13=temp::TempFactory::NewTemp();
  r14=temp::TempFactory::NewTemp();
  r15=temp::TempFactory::NewTemp();

  regs_=std::vector<temp::Temp*>{rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15};
  
  // register mapping, override origin mapping
  temp::Map::Name()->Enter(rax,new std::string("%rax"));
  temp::Map::Name()->Enter(rbx,new std::string("%rbx"));
  temp::Map::Name()->Enter(rcx,new std::string("%rcx"));
  temp::Map::Name()->Enter(rdx,new std::string("%rdx"));
  temp::Map::Name()->Enter(rsi,new std::string("%rsi"));
  temp::Map::Name()->Enter(rdi,new std::string("%rdi"));
  temp::Map::Name()->Enter(rbp,new std::string("%rbp"));
  temp::Map::Name()->Enter(rsp,new std::string("%rsp"));
  temp::Map::Name()->Enter(r8,new std::string("%r8"));
  temp::Map::Name()->Enter(r9,new std::string("%r9"));
  temp::Map::Name()->Enter(r10,new std::string("%r10"));
  temp::Map::Name()->Enter(r11,new std::string("%r11"));
  temp::Map::Name()->Enter(r12,new std::string("%r12"));
  temp::Map::Name()->Enter(r13,new std::string("%r13"));
  temp::Map::Name()->Enter(r14,new std::string("%r14"));
  temp::Map::Name()->Enter(r15,new std::string("%r15"));
}

/*
--------
args pushed on stack
--------
return address
--------rbp(call,frame pointer,begin new frame)
args on stack convey to frame(view shift)
--------
local variable
--------
*/
Frame* newFrame(temp::Label* name,std::list<bool> escapes)
{
  frame::X64Frame* frame=new frame::X64Frame();
  // not handling spill for now

  frame->name=name;
  // for static link
  frame->offset=-reg_manager->WordSize();
  temp::TempList* arg_regs=reg_manager->ArgRegs();
  int arg_max=arg_regs->GetList().size(), arg_cnt=0;
  for(const auto& arg:escapes)
  {
    // if escape, address is in register, allocate space address on stack
    frame::Access* ac=frame->AllocLocal(arg);
    frame->formals.push_back(ac);
  }
  return frame;
}

// before the program start,move value in args register to other register
// if pass address, put on stack
tree::Stm* procEntryExit1(frame::Frame* frame,tree::Stm* stm)
{
  int reg_size = reg_manager->ArgRegs()->GetList().size(),cnt=0;
  tree::Stm* tmp=new tree::ExpStm(new tree::ConstExp(0));
  for(auto& formal:frame->formals)
  {
    // to exp's arg is frame pointer
    // offset is negative ,so stack pointer minus offset
    if(cnt<reg_size)
    {
       tmp=new tree::SeqStm(
          tmp,
          new tree::MoveStm(
            formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
            new tree::TempExp(reg_manager->ArgRegs()->NthTemp(cnt))));
    }
    else
    {
      int arg_num_stk=frame->formals.size()-reg_size;// first static link
      int serial_on_stack=cnt-(reg_size-1);
      int stk_offset=(arg_num_stk-serial_on_stack)*reg_manager->WordSize()+reg_manager->WordSize();//skip return address
  
      assert(stk_offset>0);
      tmp=new tree::SeqStm(
          tmp,
          new tree::MoveStm(
            formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
            new tree::MemExp(new tree::BinopExp(tree::PLUS_OP,new tree::TempExp(reg_manager->FramePointer()),new tree::ConstExp(stk_offset)))));
    }
    cnt++;
  }
  return new tree::SeqStm(tmp,stm);
}

assem::Proc* ProcEntryExit3(frame::Frame* frame,assem::InstrList* body)
{
  std::string prolog_instr="";
  std::string set_frame=".set "+frame->name->Name()+"_framesize, "+std::to_string(-frame->offset)+"\n";
  std::string frame_label=frame->name->Name()+":\n";
  std::string sub_rsp="subq $"+std::to_string(-frame->offset)+", %rsp\n";
  prolog_instr=set_frame+frame_label+sub_rsp;

  std::string epilog_instr="";
  std::string add_rsp="addq $"+std::to_string(-frame->offset)+", %rsp\n";
  std::string ret="retq\n";
  epilog_instr=add_rsp+ret;

  return new assem::Proc(prolog_instr,body,epilog_instr);

}

} // namespace frame