#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr)const override;
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr)const override;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */

  public:
  // X64Frame(temp::Label* name);
  virtual Access* AllocLocal(bool is_local)override;
  static tree::Exp* Exp(frame::Access* access,tree::Exp* framePtr);
  static frame::Frame* NewFrame(temp::Label* name,std::list<bool> formals);

};
/* TODO: Put your lab5 code here */
Access* X64Frame::AllocLocal(bool escape)
{
  if(!escape)
  {
    frame::Access* acc = new InRegAccess(temp::TempFactory::NewTemp());
    return acc;
  }
  frame::Access* acc = new InFrameAccess(this->offset_s);
  this->locals_.push_back(acc);
  this->offset_s+=reg_manager->WordSize();
  return acc;
}

tree::Exp* X64Frame::Exp(frame::Access* access,tree::Exp* framePtr)
{
  if(typeid(*access)==typeid(frame::InFrameAccess))
  {
    frame::InFrameAccess* fa=static_cast<frame::InFrameAccess*>(access);
    return new tree::MemExp(new tree::BinopExp(tree::PLUS_OP,framePtr,new tree::ConstExp(fa->offset)));
  }
  else
  {
    frame::InRegAccess* ra=static_cast<frame::InRegAccess*>(access);
    return new tree::TempExp(ra->reg);
  }
  return nullptr;
}

frame::Frame* X64Frame::NewFrame(temp::Label* name,std::list<bool> formals)
{
  frame::Frame* x64f=new frame::X64Frame();
  x64f->name_=name;
  x64f->offset_s=0;
  for(const auto& formal:formals)
  {
    //true means escape
    frame::Access* acc=nullptr;
    if(formal)
    {
      acc = new InFrameAccess(x64f->offset_s);
      
    }
    else
    {
      acc = new InRegAccess(temp::TempFactory::NewTemp());
    }
    x64f->formals_.push_back(acc);
  }
  return nullptr;
}


tree::Exp* InFrameAccess::ToExp(tree::Exp* framePtr)const
{
  tree::Exp* e = new tree::BinopExp(tree::BinOp::MINUS_OP,framePtr,new tree::ConstExp(this->offset));
  return new tree::MemExp(e);
}

tree::Exp* InRegAccess::ToExp(tree::Exp* framePtr)const
{
  return new tree::TempExp(reg);
}

const int X64WORDSIZE=2;
// not gloabl
// static temp::Temp* RAX=temp::TempFactory::NewTemp();
// static temp::Temp* RBX=temp::TempFactory::NewTemp();
// static temp::Temp* RCX=temp::TempFactory::NewTemp();
// static temp::Temp* RDX=temp::TempFactory::NewTemp();
// static temp::Temp* RSI=temp::TempFactory::NewTemp();
// static temp::Temp* RDI=temp::TempFactory::NewTemp();
// static temp::Temp* RBP=temp::TempFactory::NewTemp();
// static temp::Temp* RSP=temp::TempFactory::NewTemp();
// static temp::Temp* R8=temp::TempFactory::NewTemp();
// static temp::Temp* R9=temp::TempFactory::NewTemp();
// static temp::Temp* R10=temp::TempFactory::NewTemp();
// static temp::Temp* R11=temp::TempFactory::NewTemp();
// static temp::Temp* R12=temp::TempFactory::NewTemp();
// static temp::Temp* R13=temp::TempFactory::NewTemp();
// static temp::Temp* R14=temp::TempFactory::NewTemp();
// static temp::Temp* R15=temp::TempFactory::NewTemp();
// static temp::Temp* RV=temp::TempFactory::NewTemp();//just a mark for return value

// //except rsi
// static temp::TempList* REGISTERS=new temp::TempList({RAX,RBX,RCX,RDX,RDI,RBP,RSP,R8,R9,R10,R11,R12,R13,R14,R15});
// static temp::TempList* ARGREGS=new temp::TempList({RDI,RSI,RDX,RCX,R8,R9});
// //except rsp
// static temp::TempList* CALLERSAVES=new temp::TempList({RAX,RBX,RCX,RDX,RSI,RDI,RBP,R8,R9,R10,R11,R12,R13,R14,R15});
// static temp::TempList* CALLEESAVES=new temp::TempList({RBX,RBP,R12,R13,R14,R15});


// temp::TempList *X64RegManager::Registers(){return REGISTERS;}
// temp::TempList *ArgRegs(){return ARGREGS;}
// temp::TempList *CallerSaves(){return CALLERSAVES;}
// temp::TempList *CalleeSaves(){return CALLEESAVES;}
// temp::TempList *ReturnSink(){return nullptr;}
// int WordSize(){return X64WORDSIZE;}
// temp::Temp *FramePointer(){return RBP;}
// temp::Temp *StackPointer(){return RSP;}
// temp::Temp *ReturnValue(){return RV;}

temp::TempList *X64RegManager::Registers(){return nullptr;}
temp::TempList *X64RegManager::ArgRegs(){return nullptr;}
temp::TempList *X64RegManager::CallerSaves(){return nullptr;}
temp::TempList *X64RegManager::CalleeSaves(){return nullptr;}
temp::TempList *X64RegManager::ReturnSink(){return nullptr;}
int X64RegManager::WordSize(){return X64WORDSIZE;}
temp::Temp *X64RegManager::FramePointer(){return nullptr;}
temp::Temp *X64RegManager::StackPointer(){return nullptr;}
temp::Temp *X64RegManager::ReturnValue(){return nullptr;}

} // namespace frame