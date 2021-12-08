#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <iostream>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

std::list<tr::Access*> Level::GetFormals()
{
  std::list<frame::Access*> frame_access=frame_->formals;
  std::list<tr::Access*> tr_access;
  for(const auto& fa:frame_access)
  {
    tr_access.push_back(new tr::Access(this,fa));
  }
  return tr_access;
}

tr::Level* NewLevel(tr::Level* parent,temp::Label* name,std::list<bool> formals)
{
  // std::cout<<"new level parent:"<<parent<<std::endl;
  formals.push_front(true); // the first for static link
  frame::Frame* frame=frame::newFrame(name,formals);
  tr::Level* level=new tr::Level(frame,parent);
  return level;
}

tr::Access* AllocLocal(tr::Level* level,bool escape)
{
  frame::Access* fa=level->frame_->AllocLocal(escape);
  return new tr::Access(level,fa);
}

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access* fa = level->frame_->AllocLocal(escape);
  return new tr::Access(level,fa);
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) 
    {
      // std::cout<<"true** addr:"<<trues_<<" true*:"<<*trues_<<" name:"<<(*trues_)->Name()<<std::endl;
    }
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    // temp::Temp* r=temp::TempFactory::NewTemp();
    // temp::Label* t=temp::LabelFactory::NewLabel();
    // temp::Label* f=temp::LabelFactory::NewLabel();
    // temp::Label** tp=new temp::Label*(t);
    // temp::Label** fp=new temp::Label*(f);
    // // std::cout<<"uncx true:"<<t<<" name:"<<t->Name()<<std::endl;
    // // std::cout<<"uncx false:"<<f<<" name:"<<f->Name()<<std::endl;
    // tree::CjumpStm* stm=new tree::CjumpStm(tree::NE_OP,exp_,new tree::ConstExp(0),t,f);
    // return Cx(tp,fp,stm);
    
    // std::cout<<"uncx true:"<<t<<" name:"<<t->Name()<<std::endl;
    // std::cout<<"uncx false:"<<f<<" name:"<<f->Name()<<std::endl;
    tree::CjumpStm* stm=new tree::CjumpStm(tree::NE_OP,exp_,new tree::ConstExp(0),nullptr,nullptr);
    temp::Label** tp=&stm->true_label_;
    temp::Label** fp=&stm->false_label_;
    // std::cout<<"ex cx tp:"<<tp<<" fp:"<<fp<<std::endl;
    return Cx(tp,fp,stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_,new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    int pos=errormsg->GetTokPos();
    errormsg->Error(pos,"conversion type error");
    return Cx(nullptr,nullptr,nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label** trues, temp::Label** falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp* r=temp::TempFactory::NewTemp();
    temp::Label* t=temp::LabelFactory::NewLabel();
    temp::Label* f=temp::LabelFactory::NewLabel();
    // std::cout<<"cx ex cxt:"<<cx_.trues_<<" cxf:"<<cx_.falses_<<std::endl;
    *(cx_.trues_)=t;
    *(cx_.falses_)=f;
 
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(1)),
      new tree::EseqExp(
        cx_.stm_,
        new tree::EseqExp(
          new tree::LabelStm(f),
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
          new tree::EseqExp(new tree::LabelStm(t),new tree::TempExp(r)))
        )
      )
    );
  }
  
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    temp::Label* label=temp::LabelFactory::NewLabel();
    // std::cout<<"cx ux cxt:"<<cx_.trues_<<" cxf:"<<cx_.falses_<<std::endl;
    *(cx_.trues_)=label;
    *(cx_.falses_)=label;
    return new tree::SeqStm(cx_.stm_,new tree::LabelStm(label)); // convert to exp without return value
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override { 
    /* TODO: Put your lab5 code here */
    // std::cout<<"cx uncx"<<std::endl;
    return cx_;
  }
};


ProgTr::ProgTr(std::unique_ptr<absyn::AbsynTree> absyntree, std::unique_ptr<err::ErrorMsg> errormsg){
  absyn_tree_=std::move(absyntree);
  errormsg_=std::move(errormsg);
}


void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  tenv_=std::make_unique<env::TEnv>();
  venv_=std::make_unique<env::VEnv>();

  // create outer most level first, which will be used by env init
  temp::Label* outer_label=temp::LabelFactory::NamedLabel("tigermain");
  tr::Level* level=new tr::Level(frame::newFrame(outer_label,std::list<bool>()),nullptr);
  main_level_.reset(level); 

  // init after main level set
  FillBaseTEnv();
  FillBaseVEnv();
  
  assert(main_level_.get()!=nullptr);
  assert(absyn_tree_.get()!=nullptr);

  // translate whole
  tr::ExpAndTy* root_et = absyn_tree_->Translate(venv_.get(),tenv_.get(),level,outer_label,errormsg_.get());

  // put outer let to frag
  tree::Stm* tmp=new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),root_et->exp_->UnEx());
  tree::Stm* wrap=frame::procEntryExit1(level->frame_,tmp);
  frags->PushBack(new frame::ProcFrag(wrap,level->frame_));
}

tree::Exp* make_staticlink(tr::Level* target, tr::Level* level)
{
  tree::Exp* static_link=new tree::TempExp(reg_manager->FramePointer());

  while(level!=target)
  {
    static_link=level->frame_->formals.front()->ToExp(static_link);
    level=level->parent_;
  }

  return static_link;
}

tr::Exp* simpleVar(tr::Access* access,tr::Level* level)
{
  tree::Exp* static_link=make_staticlink(access->level_,level);
  static_link = access->access_->ToExp(static_link);
  return new tr::ExExp(static_link);
}


tr::Exp* subscriptVar(tr::Exp* var,tr::Exp* subscript)
{
  return new tr::ExExp(
    new tree::MemExp(
      new tree::BinopExp(
        tree::PLUS_OP,var->UnEx(),
        new tree::BinopExp(
          tree::MUL_OP,subscript->UnEx(),new tree::ConstExp(reg_manager->WordSize())
        ))));
}

tr::Exp* intOperation(tr::Exp* left,tr::Exp* right,absyn::Oper op)
{
  switch(op)
  {
    case absyn::Oper::PLUS_OP:
    {
      return new tr::ExExp(new tree::BinopExp(tree::PLUS_OP,left->UnEx(),right->UnEx()));
    }
    case absyn::Oper::MINUS_OP:
    {
      return new tr::ExExp(new tree::BinopExp(tree::MINUS_OP,left->UnEx(),right->UnEx()));
    }
    case absyn::Oper::TIMES_OP:
    {
      return new tr::ExExp(new tree::BinopExp(tree::MUL_OP,left->UnEx(),right->UnEx()));
    }
    case absyn::Oper::DIVIDE_OP:
    {
      return new tr::ExExp(new tree::BinopExp(tree::DIV_OP,left->UnEx(),right->UnEx()));
    }
  }
}

// default left and right can be compared
tr::Exp* intCompare(tr::Exp* left,tr::Exp* right,absyn::Oper op)
{
  // temp::Label* t=temp::LabelFactory::NewLabel();
  // temp::Label* f=temp::LabelFactory::NewLabel();
  // temp::Label** tp=new temp::Label*(t);
  // temp::Label** fp=new temp::Label*(f);
  tree::CjumpStm* cjumpstm=nullptr;
  switch(op)
  {
    case absyn::Oper::EQ_OP:
    {
      // std::cout<<"int cmp eq"<<std::endl;
      cjumpstm= new tree::CjumpStm(tree::EQ_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
      // stm = new tree::CjumpStm(tree::EQ_OP,left->UnEx(),right->UnEx(),t,f);
      // break;
    }
    case absyn::Oper::NEQ_OP:
    {
      // std::cout<<"int cmp not eq"<<std::endl;
      cjumpstm= new tree::CjumpStm(tree::NE_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
    }
    case absyn::Oper::LT_OP:
    {
      cjumpstm= new tree::CjumpStm(tree::LT_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
    }
    case absyn::Oper::LE_OP:
    {
      cjumpstm= new tree::CjumpStm(tree::LE_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
    }
    case absyn::Oper::GT_OP:
    {
      cjumpstm= new tree::CjumpStm(tree::GT_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
    }
    case absyn::Oper::GE_OP:
    {
      cjumpstm= new tree::CjumpStm(tree::GE_OP,left->UnEx(),right->UnEx(),nullptr,nullptr);
      break;
    }
    default: assert(0);
  }
  temp::Label** tp=&(cjumpstm->true_label_);
  temp::Label** fp=&(cjumpstm->false_label_);
  // std::cout<<"tp:"<<tp<<" fp:"<<fp<<std::endl;
  return new tr::CxExp(tp,fp,cjumpstm);
}

tr::Exp* strCompare(tr::Exp* left,tr::Exp* right,absyn::Oper op)
{
  // temp::Label* t=temp::LabelFactory::NewLabel();
  // temp::Label* f=temp::LabelFactory::NewLabel();
  // temp::Label** tp=new temp::Label*(t);
  // temp::Label** fp=new temp::Label*(f);
  tree::CjumpStm* cjumpstm=nullptr;
  switch(op)
  {
    case absyn::Oper::EQ_OP:
    {
      // runtime function string not equal 0,else 1
      cjumpstm=new tree::CjumpStm(
          tree::EQ_OP,
          new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("string_equal")),new tree::ExpList({left->UnEx(),right->UnEx()})),
          new tree::ConstExp(1),
          nullptr,nullptr);
      break;
    }
    default:assert(0);
  }
  temp::Label** tp=&(cjumpstm->true_label_);
  temp::Label** fp=&(cjumpstm->false_label_);
  return new tr::CxExp(tp,fp,cjumpstm);
}

tr::Exp* otherCompare(tr::Exp* left,tr::Exp* right,absyn::Oper op)
{
  // temp::Label* t=temp::LabelFactory::NewLabel();
  // temp::Label* f=temp::LabelFactory::NewLabel();
  // temp::Label** tp=new temp::Label*(t);
  // temp::Label** fp=new temp::Label*(f);
  tree::CjumpStm* cjumpstm=nullptr;
  switch(op)
  {
    case absyn::Oper::EQ_OP:
    {
      // runtime function string not equal 0,else 1
      cjumpstm= new tree::CjumpStm(
          tree::EQ_OP,
          left->UnEx(),
          right->UnEx(),
          nullptr,nullptr);
      break;
    }
    case absyn::Oper::NEQ_OP:
    {
      // runtime function string not equal 0,else 1
      cjumpstm=new tree::CjumpStm(
          tree::NE_OP,
          left->UnEx(),
          right->UnEx(),
          nullptr,nullptr);
      break;
    }
    default:assert(0);
  }
  temp::Label** tp=&(cjumpstm->true_label_);
  temp::Label** fp=&(cjumpstm->false_label_);
  return new tr::CxExp(tp,fp,cjumpstm);
}

tr::Exp* make_if(tr::Exp* test,tr::Exp* then,tr::Exp* elsee,err::ErrorMsg *errormsg)
{
  Cx cx = test->UnCx(errormsg);
  temp::Temp* r=temp::TempFactory::NewTemp(); // temp register
  temp::Label* t=temp::LabelFactory::NewLabel(); // true
  temp::Label* f=temp::LabelFactory::NewLabel(); // false
  temp::Label* d=temp::LabelFactory::NewLabel(); // dome
  *(cx.trues_)=t; // true
  *(cx.falses_)=f; // false
 
  if(elsee)
  {
    return new tr::ExExp(
      new tree::EseqExp(cx.stm_,
        new tree::EseqExp(new tree::LabelStm(t),
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),then->UnEx()),
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(d),new std::vector<temp::Label *>({d})),
              new tree::EseqExp(new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),elsee->UnEx()),
                  new tree::EseqExp(new tree::JumpStm(new tree::NameExp(d),new std::vector<temp::Label *>({d})),
                    new tree::EseqExp(new tree::LabelStm(d),new tree::TempExp(r))))))))));
  }
  // no value
  else
  {
    return new tr::NxExp(
      new tree::SeqStm(cx.stm_,
        new tree::SeqStm(new tree::LabelStm(t),
          new tree::SeqStm(then->UnNx(),new tree::LabelStm(f)))));
  }
  assert(0);
}

tr::Exp* make_while(tr::Exp* test,tr::Exp* body,temp::Label* done, err::ErrorMsg *errormsg)
{
  Cx condition=test->UnCx(errormsg);
  temp::Label* test_label=temp::LabelFactory::NewLabel();
  temp::Label* body_label=temp::LabelFactory::NewLabel();
  // std::cout<<"ct:"<<condition.trues_<<" cf:"<<condition.falses_<<std::endl;
  *(condition.trues_)=body_label; // true
  *(condition.falses_)=done;  // false

  
  return new tr::NxExp(
    new tree::SeqStm(new tree::LabelStm(test_label),
      new tree::SeqStm(condition.stm_,
        new tree::SeqStm(new tree::LabelStm(body_label),
          new tree::SeqStm(body->UnNx(),
            new tree::SeqStm(new tree::JumpStm(new tree::NameExp(test_label), new std::vector<temp::Label *>({test_label})),
              new tree::LabelStm(done)))))));
  
  // guarded-do
  // return new tr::NxExp(
  //   new tree::SeqStm(test->UnCx(errormsg).stm_,
  //     new tree::SeqStm(new tree::LabelStm(loop),
  //       new tree::SeqStm(body->UnCx(errormsg).stm_,
  //         new tree::SeqStm(test->UnCx(errormsg).stm_,
  //           new tree::LabelStm(done))))));
}

tr::Exp* make_for(frame::Access* access,tr::Exp* lo,tr::Exp* hi,tr::Exp* body, temp::Label* done,err::ErrorMsg *errormsg)
{
  tree::Exp* i=frame::exp(access,new tree::TempExp(reg_manager->FramePointer()));
  temp::Label* body_label=temp::LabelFactory::NewLabel();
  temp::Label* test=temp::LabelFactory::NewLabel();
  // temp::Label* done=temp::LabelFactory::NewLabel();
 return new tr::NxExp(
      new tree::SeqStm(new tree::MoveStm(i, lo->UnEx()),
      new tree::SeqStm(new tree::LabelStm(test),
       new tree::SeqStm(new tree::CjumpStm(tree::LE_OP, i, hi->UnEx(), body_label, done),
        new tree::SeqStm(new tree::LabelStm(body_label),
         new tree::SeqStm(body->UnNx(),
          new tree::SeqStm(new tree::MoveStm(i, new tree::BinopExp(tree::PLUS_OP, i, new tree::ConstExp(1))),
           new tree::SeqStm(new tree::JumpStm(new tree::NameExp(test), new std::vector<temp::Label *>({test})),
            new tree::LabelStm(done)))))))));
}

tr::Exp* make_seq(tr::Exp* left,tr::Exp* right)
{
  if(right)
  {
    return new tr::ExExp(new tree::EseqExp(left->UnNx(),right->UnEx()));
  }
  else{
    return new tr::ExExp(new tree::EseqExp(left->UnNx(),new tree::ConstExp(0)));
  }
}

tr::Exp* make_nil()
{
  // just generate an unnecessary assem
  return new tr::ExExp(new tree::ConstExp(0));
}


} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->root_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry* ee=venv->Look(this->sym_);
  if(ee==nullptr||typeid(*ee)!=typeid(env::VarEntry))
  {
    errormsg->Error(this->pos_,"translation error");
    return new tr::ExpAndTy(nullptr,nullptr);
  }
  env::VarEntry* ve=static_cast<env::VarEntry*>(ee);
  return new tr::ExpAndTy(tr::simpleVar(ve->access_,level),ve->ty_->ActualTy());
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* ty_et=var_->Translate(venv,tenv,level,label,errormsg);
  if(ty_et==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->sym_);
    return nullptr;
  }
  else if(typeid(*(ty_et->ty_))!=typeid(type::RecordTy))
  {
    errormsg->Error(this->pos_,"not a record type");
    return nullptr;
  }
  type::RecordTy* rt=static_cast<type::RecordTy*>(ty_et->ty_);

  std::list<type::Field*> type_field=rt->fields_->GetList();
  int offset=0;
  for(const auto& f:type_field)
  {
    if(f->name_->Name()==this->sym_->Name())
    {
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::MemExp(
            new tree::BinopExp(tree::PLUS_OP,ty_et->exp_->UnEx(),new tree::ConstExp(offset*reg_manager->WordSize())))),
        f->ty_->ActualTy());
    }
    ++offset;
  }
  assert(0);
  return nullptr;
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy * var_et = this->var_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* subscript_et=this->subscript_->Translate(venv,tenv,level,label,errormsg);
  type::Ty* ty=var_et->ty_->ActualTy();
  if(ty&&typeid(*ty)!=typeid(type::ArrayTy))
  {
    errormsg->Error(this->pos_,"array type required");
    return nullptr;
  }
  type::ArrayTy* real_ty=static_cast<type::ArrayTy*>(ty);
  type::Ty* ret_ty=real_ty->ty_->ActualTy();
  return new tr::ExpAndTy(tr::subscriptVar(var_et->exp_,subscript_et->exp_),ret_ty);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(0)),type::NilTy::Instance()
    );
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(this->val_)),type::IntTy::Instance()
      );
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label* str_label=temp::LabelFactory::NewLabel();
  frame::StringFrag* str_frag=new frame::StringFrag(str_label,this->str_);
  frags->PushBack(str_frag);
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::NameExp(str_label)),type::StringTy::Instance()
  );
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry* env_entry = venv->Look(func_);

  if(env_entry==nullptr||typeid(*env_entry)!=typeid(env::FunEntry))
  {
    errormsg->Error(this->pos_,"undefined function %s",this->func_->Name().data());
    return nullptr;
  }

  env::FunEntry* fun_entry = static_cast<env::FunEntry*>(env_entry);
  tree::ExpList* args=new tree::ExpList();

  std::list<type::Ty*> formal_list =  fun_entry->formals_->GetList();
  std::list<absyn::Exp*> actual_list = this->args_->GetList();

  auto formals_it=formal_list.begin(),formals_end=formal_list.end();
  auto call_it=actual_list.begin(),call_end=actual_list.end();

  if(actual_list.size()<formal_list.size())
  {
    errormsg->Error(this->pos_,"too few params in function %s",this->func_->Name().data());
  }
  else if(actual_list.size()>formal_list.size())
  {
    errormsg->Error(this->pos_,"too many params in function %s",this->func_->Name().data());
  }

  for(;formals_it!=formals_end&&call_it!=call_end;++formals_it,++call_it)
  {
    tr::ExpAndTy* call_et = (*call_it)->Translate(venv,tenv,level,label,errormsg);
    if(!(*formals_it)->ActualTy()->IsSameType(call_et->ty_))
    {
      errormsg->Error(this->pos_,"para type mismatch");
    }
    args->Append(call_et->exp_->UnEx());
  }

  if(formals_it!=formals_end||call_it!=call_end)
  {
    errormsg->Error(this->pos_,"function call error");
  }


  if(fun_entry->level_->parent_)
  {
    tree::Exp* static_link=tr::make_staticlink(fun_entry->level_->parent_,level);
    args->Insert(static_link);
    return new tr::ExpAndTy(
      new tr::ExExp(new tree::CallExp(new tree::NameExp(this->func_),args)),fun_entry->result_
    );
  }
  else
  {
    // main function
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::CallExp(
          new tree::NameExp(temp::LabelFactory::NamedLabel(temp::LabelFactory::LabelString(this->func_))),args)),fun_entry->result_
    );
  }
  assert(0);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* l_et=this->left_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* r_et=this->right_->Translate(venv,tenv,level,label,errormsg);
  type::Ty* lt=l_et->ty_, *rt=r_et->ty_;
  switch(this->oper_)
  {
    case  PLUS_OP: case MINUS_OP: case TIMES_OP: case DIVIDE_OP:
    {
      if(lt==nullptr||rt==nullptr||
      typeid(*lt)!=typeid(type::IntTy)||
      typeid(*rt)!=typeid(type::IntTy))
      {
        errormsg->Error(this->pos_,"integer required");
        return nullptr;
      }
      return new tr::ExpAndTy(tr::intOperation(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
    }
    case LT_OP: case LE_OP: case GT_OP: case GE_OP:
    {
      if((typeid(*(lt->ActualTy()))==typeid(type::IntTy))
        &&(typeid(*(rt->ActualTy()))==typeid(type::IntTy)))
      {
        return new tr::ExpAndTy(tr::intCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
      else
      {
        // errormsg->Error(this->pos_,"type:%s type:%s\n",typeid(*lt).name(),typeid(*rt).name());
        errormsg->Error(this->pos_,"same type required");
      }
      break;
    }
    case EQ_OP: 
    {
      if((typeid(*(lt->ActualTy()))==typeid(type::IntTy))
        &&(typeid(*(rt->ActualTy()))==typeid(type::IntTy)))
      {
        return new tr::ExpAndTy(tr::intCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
      else if((typeid(*(l_et->ty_))==typeid(type::StringTy))
        &&(typeid(*(r_et->ty_))==typeid(type::StringTy)))
      {
        return new tr::ExpAndTy(tr::strCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
      else
      {
        return new tr::ExpAndTy(tr::otherCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
    }
    case NEQ_OP: 
    {
      if((typeid(*(lt->ActualTy()))==typeid(type::IntTy))
        &&(typeid(*(rt->ActualTy()))==typeid(type::IntTy)))
      {
        return new tr::ExpAndTy(tr::intCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
      else 
      {
        return new tr::ExpAndTy(tr::otherCompare(l_et->exp_,r_et->exp_,this->oper_),type::IntTy::Instance());
      }
    }
    default:assert(0);
  }
  return nullptr;
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* ty = tenv->Look(this->typ_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->typ_->Name().data());
    return nullptr;
  }
  ty=ty->ActualTy();
  if(typeid(*ty)!=typeid(type::RecordTy))
  {
    errormsg->Error(this->pos_,"not record type:%s",typeid(*ty).name());
  }

  type::RecordTy* rty=static_cast<type::RecordTy*>(ty);
  std::list<type::Field*> fl = rty->fields_->GetList();
  std::list<absyn::EField*> efl = this->fields_->GetList();
  tree::ExpList* exp_list=new tree::ExpList();
  auto fl_it=fl.begin(),fl_end=fl.end();
  auto efl_it=efl.begin(),efl_end=efl.end();
  for(;fl_it!=fl_end&&efl_it!=efl_end;++fl_it,++efl_it)
  {
    if((*fl_it)->name_->Name()!=(*efl_it)->name_->Name())
      errormsg->Error(this->pos_,"same type required");
    tr::ExpAndTy* efl_et=(*efl_it)->exp_->Translate(venv,tenv,level,label,errormsg);
    type::Ty* fl_type=(*fl_it)->ty_;
    if(efl_et&&fl_type&&!fl_type->IsSameType(efl_et->ty_))
    {
      // errormsg->Error(this->pos_,"fun:%s call:%s",typeid(*fl_type).name(),typeid(*efl_type).name());
      errormsg->Error(this->pos_,"same type required");
    }
    exp_list->Append(efl_et->exp_->UnEx());
  }
  if(fl_it!=fl_end||efl_it!=efl_end)
    errormsg->Error(this->pos_,"record element num error fl:%d efl:%d",fl.size(),efl.size());

  temp::Temp* base_reg=temp::TempFactory::NewTemp(); // register store base address
  tree::Stm* stm = new tree::MoveStm(new tree::TempExp(base_reg),
    new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel("alloc_record")),
      new tree::ExpList({new tree::ConstExp(exp_list->GetList().size()*reg_manager->WordSize())})));
  std::list<tree::Exp*> item_list = exp_list->GetList();
  int count=0;
  for(const auto& item:item_list)
  {
    stm=new tree::SeqStm(stm,
      new tree::MoveStm(
        new tree::MemExp(new tree::BinopExp(tree::PLUS_OP,new tree::TempExp(base_reg),new tree::ConstExp(count*reg_manager->WordSize()))),
        item
      ));
    count++;
  }

  return new tr::ExpAndTy(
    new tr::ExExp(new tree::EseqExp(stm,new tree::TempExp(base_reg))),ty->ActualTy());
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Exp*> el = this->seq_->GetList();
  tr::ExpAndTy* cur_seq=nullptr;
  tr::Exp* exp=tr::make_nil();

  for(const auto& e:el)
  {
    cur_seq=e->Translate(venv,tenv,level,label,errormsg);
    assert(cur_seq!=nullptr);
    exp = tr::make_seq(exp,cur_seq->exp_);
  }
  return new tr::ExpAndTy(exp,cur_seq->ty_->ActualTy());
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* var_et = this->var_->Translate(venv, tenv, level, label,errormsg);
  tr::ExpAndTy* exp_et = this->exp_->Translate(venv, tenv, level, label,errormsg);
  type::Ty* vty=var_et->ty_->ActualTy();
  type::Ty* ety=exp_et->ty_->ActualTy();
  // cannot assign to readonly id
  if(venv->IsWithinScope(sym::scope_::FOR_SCOPE))
  {
    if(this->var_&&typeid(*(this->var_))==typeid(absyn::SimpleVar))
    {
      absyn::SimpleVar* sv=static_cast<absyn::SimpleVar*>(this->var_);
      env::EnvEntry* sv_en = venv->Look(sv->sym_);
      if(sv_en->readonly_)
      {
        errormsg->Error(this->pos_,"loop variable can't be assigned");
      }
    }
  }
  
  // if(vty&&ety&&typeid(*vty)!=typeid(*ety))
  // {
  //   // std::cout<<" "<<typeid(*vty).name()<<" "<<typeid(*ety).name()<<std::endl;
  //   errormsg->Error(this->pos_,"unmatched assign exp");
  // }
  
  return new tr::ExpAndTy(
    new tr::NxExp(new tree::MoveStm(var_et->exp_->UnEx(),exp_et->exp_->UnEx()))
    ,type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* test_et=this->test_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* then_et=this->then_->Translate(venv,tenv,level,label,errormsg);
  type::Ty* test_ty=test_et->ty_;
  type::Ty* then_ty=then_et->ty_;
  if(this->elsee_==nullptr)
  {
    if(then_ty&&!type::VoidTy::Instance()->IsSameType(then_ty))
    {
      errormsg->Error(this->then_->pos_,"if-then exp's body must produce no value");
      return nullptr;
    }
    return new tr::ExpAndTy(tr::make_if(test_et->exp_,then_et->exp_,nullptr,errormsg),then_ty);
  }
  else
  {
    tr::ExpAndTy* elsee_et=this->elsee_->Translate(venv,tenv,level,label,errormsg);
    type::Ty* elsee_ty=elsee_et->ty_;
    if(elsee_ty&&elsee_ty->IsSameType(then_ty))
    {
      return new tr::ExpAndTy(tr::make_if(test_et->exp_,then_et->exp_,elsee_et->exp_,errormsg),then_ty);
    }
    else
    {
      errormsg->Error(this->then_->pos_,"then exp and else exp type mismatch");
    }
  }
  return nullptr;
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* test_et=this->test_->Translate(venv,tenv,level,label,errormsg);
  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy* body_et=this->body_->Translate(venv,tenv,level,done,errormsg);
  type::Ty* test_ty=test_et->ty_;
  type::Ty* body_ty=body_et->ty_;
  if(test_ty&&!type::IntTy::Instance()->IsSameType(test_ty))
    errormsg->Error(this->test_->pos_,"integer required!");
  if(body_ty&&!type::VoidTy::Instance()->IsSameType(body_ty))
    errormsg->Error(this->body_->pos_,"while body must produce no value");
  return new tr::ExpAndTy(tr::make_while(test_et->exp_,body_et->exp_,done,errormsg),type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope(sym::PLAIN_SCOPE);
  tr::ExpAndTy* lt_et=this->lo_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* ht_et=this->hi_->Translate(venv,tenv,level,label,errormsg);
  
  type::Ty* lt_ty=lt_et->ty_;
  type::Ty* ht_ty=ht_et->ty_;

  tr::Access* access = tr::AllocLocal(level,this->escape_);
  venv->Enter(this->var_,new env::VarEntry(access,lt_ty,true));

  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy* body_et=this->body_->Translate(venv,tenv,level,done,errormsg);
  type::Ty* body_ty=body_et->ty_;

  if(ht_ty!=nullptr&&typeid(*ht_ty)!=typeid(type::IntTy))
    errormsg->Error(this->pos_,"for exp's range type is not integer");
  if(body_ty&&!type::VoidTy::Instance()->IsSameType(body_ty))
    errormsg->Error(this->body_->pos_,"body must be no type");

  venv->EndScope();

  // tree::Exp* i = frame::exp(access->access_,new tree::TempExp(reg_manager->FramePointer()));

  return new tr::ExpAndTy(tr::make_for(access->access_,lt_et->exp_,ht_et->exp_,body_et->exp_,done,errormsg),type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::NxExp(new tree::JumpStm(new tree::NameExp(label),new std::vector<temp::Label *>({label})))
    ,type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  assert(decs_!=nullptr);
  std::list<Dec *> dl=this->decs_->GetList();
  tree::Exp* seq=nullptr;
  tree::Stm* stm=nullptr;

  // std::cout<<"dl size:"<<dl.size()<<std::endl;

  // handle dec, dec is not exp
  if(dl.size()>0)
  {
    stm=(dl.front())->Translate(venv,tenv,level,label,errormsg)->UnNx();
    int cnt=0;
    for(const auto& d:dl)
    {
      // std::cout<<cnt<<" type:"<<typeid(*d).name()<<std::endl;
      if(cnt==0){
        ++cnt;
        continue;
      }
      // std::cout<<"before"<<std::endl;
      stm=new tree::SeqStm(stm,d->Translate(venv,tenv,level,label,errormsg)->UnNx());
      // std::cout<<"end"<<std::endl;
      ++cnt;
    }

  }
  // std::cout<<"body"<<std::endl;
  tr::ExpAndTy* body_et=nullptr;
  if(this->body_) body_et = this->body_->Translate(venv,tenv,level,label,errormsg);
  if(stm) seq = new tree::EseqExp(stm,body_et->exp_->UnEx());
  else seq = body_et->exp_->UnEx(); 
  return new tr::ExpAndTy(new tr::ExExp(seq), body_et->ty_->ActualTy());
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* tt=tenv->Look(this->typ_);
  if(tt==nullptr)
  {
    errormsg->Error(this->pos_,"type not exist");
    return nullptr;
  }
  // get actual type from name type
  tt=tt->ActualTy();
  if(typeid(*tt)!=typeid(type::ArrayTy))
  {
    errormsg->Error(this->pos_,"type error:%s",typeid(*tt).name());
    return nullptr;
  }
  
  tr::ExpAndTy* st=this->size_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* it=this->init_->Translate(venv,tenv,level,label,errormsg);
  if(typeid(*(st->ty_))!=typeid(type::IntTy))
  {
    errormsg->Error(this->pos_,"array size must be int");
    return nullptr;
  }

  type::ArrayTy* at_it=static_cast<type::ArrayTy*>(tt);
  if(!(it->ty_)->IsSameType((at_it->ty_)))
  {
    errormsg->Error(this->pos_,"type mismatch");
    return nullptr;
  }
  
  tree::Exp* call = new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel("init_array")),
      new tree::ExpList({st->exp_->UnEx(),it->exp_->UnEx()})
  );
  return new tr::ExpAndTy(new tr::ExExp(call),tt);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr,type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  std::list<FunDec*> fl=this->functions_->GetList();
  for(const auto& function:fl)
  {

    //check name is same
    if(nullptr!=venv->Look(function->name_))
    {
      errormsg->Error(this->pos_,"two functions have the same name");
    }
    type::TyList* ftl = function->params_->MakeFormalTyList(tenv,errormsg);
    std::list<absyn::Field*> param_list = function->params_->GetList();
    temp::Label* name=temp::LabelFactory::NamedLabel(function->name_->Name());
    std::list<bool> args_escape;

    for(const auto& field:param_list)
    {
      args_escape.push_back(field->escape_);
    }


    // frame::Frame* frame=frame::newFrame(name,args_escape);
    // tr::Level* new_level=new tr::Level(frame,level);
    tr::Level* new_level=tr::NewLevel(level,name,args_escape);
    // std::cout<<"new level:"<<new_level<<" parent:"<<new_level->parent_<<std::endl;
    type::Ty* result_type=type::VoidTy::Instance();
    if(function->result_) result_type = tenv->Look(function->result_);
    venv->Enter(function->name_,new env::FunEntry(new_level,name,ftl,result_type));
  }

  for(const auto& function:fl)
  {
    env::EnvEntry *entry = venv->Look(function->name_);
    assert(typeid(*entry)==typeid(env::FunEntry));
    env::FunEntry* fun_entry=static_cast<env::FunEntry*>(entry);
    //function in new scope
    venv->BeginScope(sym::scope_::PLAIN_SCOPE);

    std::list<frame::Access*> access_list=fun_entry->level_->frame_->formals;
    std::list<type::Ty *> formals_list=fun_entry->formals_->GetList();
    std::list<absyn::Field*> name_list = function->params_->GetList();

    auto formal_it=formals_list.begin();
    auto formal_end=formals_list.end();
    auto access_it=(++access_list.begin());//escape static link
    auto name_it=name_list.begin();
    
    // assert(access_list.size()==formals_list.size());

    for(;formal_it!=formal_end;formal_it++,access_it++,name_it++)
    {
      //check it
      venv->Enter((*name_it)->name_,new env::VarEntry(new tr::Access(fun_entry->level_,*access_it),*formal_it));
    }

    tr::ExpAndTy* body_et=function->body_->Translate(venv,tenv,fun_entry->level_,fun_entry->label_,errormsg);
    type::Ty* ty=body_et->ty_;
    //check whether procedure returns value
    if(function->result_==nullptr&&ty&&typeid(*ty)!=typeid(type::VoidTy))
    {
      errormsg->Error(this->pos_,"procedure returns value");
    }
    venv->EndScope();

    // move the whole body to return value register
    tree::Stm* tmp=new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),body_et->exp_->UnEx());
    tree::Stm* wrap=frame::procEntryExit1(fun_entry->level_->frame_,tmp);
    frags->PushBack(new frame::ProcFrag(wrap,fun_entry->level_->frame_));
  }

  return tr::make_nil();
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* init_et=init_->Translate(venv,tenv,level,label,errormsg);
  type::Ty* ty=init_et->ty_;
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type ty");
  }
  // if(this->typ_)
  // {
  //   type::Ty* t = tenv->Look(this->typ_);
  //   if(t&&ty&&!t->IsSameType(ty))
  //     errormsg->Error(this->pos_,"type mismatch");
  // }
  // else
  // {
  //   //type is null
  //   if(ty&&typeid(*ty)==typeid(type::NilTy))
  //   {
  //     errormsg->Error(this->pos_,"init should not be nil without type specified");
  //   }
  // }
  //check var name is same
  if(nullptr!=venv->Look(this->var_))
  {
    errormsg->Error(this->pos_,"two vars have the same name");
  }
  // allocate new var of the frame
  tr::Access* acc=tr::Access::AllocLocal(level,this->escape_);
  venv->Enter(this->var_, new env::VarEntry(acc,ty));
  return new tr::NxExp(
    new tree::MoveStm(tr::simpleVar(acc,level)->UnEx(),init_et->exp_->UnEx())
  );
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy *> ntl=this->types_->GetList();
  //put the head into tenv
  for(const auto& nt:ntl)
  {
    if(nullptr!=tenv->Look(nt->name_))
    {
      errormsg->Error(this->pos_,"two types have the same name");
    }
    tenv->Enter(nt->name_,new type::NameTy(nt->name_,nullptr));
  }

  for(const auto& nt:ntl)
  {
    type::Ty* head=tenv->Look(nt->name_);
    assert(typeid(*head)==typeid(type::NameTy));
    type::NameTy* name_ty=static_cast<type::NameTy*>(head);
    name_ty->ty_=nt->ty_->Translate(tenv,errormsg);
  }

  for(const auto& nt:ntl)
  {
    // tenv->Set(nt->name_,nt->ty_->Translate(tenv,errormsg));
    //detect illegel cycle: fast pointer and slow pointer
    type::Ty* head=tenv->Look(nt->name_);
    type::Ty* fast=head,*slow=head;


    // while(fast!=nullptr&&
    // typeid(*fast)==typeid(type::NameTy)&&
    // tenv->Look(static_cast<type::NameTy*>(fast)->sym_)!=nullptr&&
    // typeid(*(tenv->Look(static_cast<type::NameTy*>(fast)->sym_)))==typeid(type::NameTy))
    // {
    //   slow=tenv->Look(static_cast<type::NameTy*>(slow)->sym_);
    //   type::Ty* fast_next=tenv->Look(static_cast<type::NameTy*>(fast)->sym_);
    //   fast=tenv->Look(static_cast<type::NameTy*>(fast_next)->sym_);
    //   if(slow==fast)
    //   {
    //     errormsg->Error(this->pos_,"illegal type cycle");
    //     return nullptr;
    //   }
    // }
  }
  return tr::make_nil();
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* ty = tenv->Look(this->name_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->name_);
  }
  return new type::NameTy(this->name_,ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList* fll=new type::FieldList();
  type::RecordTy* record_ty=new type::RecordTy(fll);
  std::list<absyn::Field *> fl=this->record_->GetList();
 
  for(const auto& f:fl)
  {
    type::Ty* ty = tenv->Look(f->typ_);

    if(ty==nullptr)
    {
      errormsg->Error(this->pos_,"undefined type %s",f->typ_->Name().data());
    }

    //empty record type for now
    if(ty&&typeid(*ty)==typeid(type::NameTy))
    {
      type::NameTy* nt=static_cast<type::NameTy*>(ty);
      //it is a recursive type
      if(nt->ty_==nullptr&&nt->sym_->Name()==f->typ_->Name())
      {
        fll->Append(new type::Field(f->name_,record_ty));
      }
      //it is exactly a name type
      if(nt->ty_!=nullptr)
      {
        fll->Append(new type::Field(f->name_,ty));
      }
    }
    else{
      fll->Append(new type::Field(f->name_,ty));
    }
  }
  record_ty->fields_=fll;
  return record_ty;
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* ty = tenv->Look(this->array_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"no such type");
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn
