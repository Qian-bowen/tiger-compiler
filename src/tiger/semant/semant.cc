#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  int labelcount=0;
  root_->SemAnalyze(venv,tenv,labelcount,errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  sym::Symbol* s=this->sym_;
  env::EnvEntry* ee=venv->LookScope(s);
  if(ee && typeid(*ee)==typeid(env::VarEntry))
  {
    return (static_cast<env::VarEntry*>(ee))->ty_;
  }
  else{
    errormsg->Error(this->pos_,"undefined variable %s",this->sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //lvalue.id
  type::Ty* ty=this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->sym_);
    return nullptr;
  }
  else if(typeid(*ty)!=typeid(type::RecordTy))
  {
    errormsg->Error(this->pos_,"not a record type");
    return nullptr;
  }
  type::RecordTy* rt=static_cast<type::RecordTy*>(ty);
  if(rt==nullptr)
  {
    return nullptr;
  }
  
  std::list<type::Field*> type_field=rt->fields_->GetList();
  for(const auto& f:type_field)
  {
    if(f->name_->Name()==this->sym_->Name())
    {
      return f->ty_;
    }
  }
  errormsg->Error(this->pos_,"field %s doesn't exist",this->sym_->Name().data());
  return nullptr;
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* var_type = this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(var_type&&(typeid(*var_type)!=typeid(type::ArrayTy)))
  {
    errormsg->Error(this->pos_,"array type required");
    return nullptr;
  }
  type::Ty* ty = this->subscript_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(ty&&(typeid(*ty)!=typeid(type::IntTy)))
    errormsg->Error(this->pos_,"exp should be int");
  type::ArrayTy* at=static_cast<type::ArrayTy*>(var_type);
  return at->ty_;
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<absyn::Exp*> el = this->args_->GetList();
  env::EnvEntry* ee = venv->LookScope(this->func_);
  if(ee==nullptr)
  {
    errormsg->Error(this->pos_,"undefined function %s",this->func_->Name().data());
    return nullptr;
  }
  if(typeid(*ee)!=typeid(env::FunEntry))
  {
    errormsg->Error(this->pos_,"function dec missing");
  }
  env::FunEntry* fe=static_cast<env::FunEntry*>(ee);
  std::list<type::Ty*> tyl = fe->formals_->GetList();
  //function args type
  auto formals_it=tyl.begin(),formals_end=tyl.end();
  //call exp
  auto call_it=el.begin(),call_end=el.end();

  //cheak param numbers
  if(el.size()<tyl.size())
  {
    errormsg->Error(this->pos_,"too few params in function %s",this->func_->Name().data());
  }
  else if(el.size()>tyl.size())
  {
    errormsg->Error(this->pos_,"too many params in function %s",this->func_->Name().data());
  }

  for(;formals_it!=formals_end&&call_it!=call_end;++formals_it,++call_it)
  {
    type::Ty* call_ty = (*call_it)->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty* formals_type=(*formals_it);
    if(call_ty&&formals_type&&!call_ty->IsSameType(formals_type))
    {
      errormsg->Error(this->pos_,"call:%s dec:%s",typeid(*(call_ty->ActualTy())).name(),typeid(*(formals_type->ActualTy())).name());
      errormsg->Error(this->pos_,"para type mismatch");
    }
  }
  if(formals_it!=formals_end||call_it!=call_end)
  {
    errormsg->Error(this->pos_,"function call error");
  }
  return fe->result_;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty * lt=this->left_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty * rt=this->right_->SemAnalyze(venv,tenv,labelcount,errormsg);
  absyn::Oper op=this->oper_;
  
  switch(op)
  {
    case  PLUS_OP: case MINUS_OP: case TIMES_OP: case DIVIDE_OP:
    {
      if(lt==nullptr||rt==nullptr||
      typeid(*lt)!=typeid(type::IntTy)||
      typeid(*rt)!=typeid(type::IntTy))
      {
        errormsg->Error(this->pos_,"integer required");
      }
      break;
    }
    case EQ_OP: case NEQ_OP: case LT_OP: case LE_OP: case GT_OP: case GE_OP:
    {
      if(lt&&rt&&!lt->IsSameType(rt))
      {
        // errormsg->Error(this->pos_,"type:%s type:%s\n",typeid(*lt).name(),typeid(*rt).name());
        errormsg->Error(this->pos_,"same type required");
      }
      break;
    }
    default:
      break;
  }

  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* ty = tenv->LookScope(this->typ_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->typ_->Name().data());
    return nullptr;
  }
  if(ty!=nullptr&&typeid(*ty)!=typeid(type::RecordTy))
    errormsg->Error(this->pos_,"same type required");

  type::RecordTy* rty=static_cast<type::RecordTy*>(ty);
  std::list<type::Field*> fl = rty->fields_->GetList();
  std::list<absyn::EField*> efl = this->fields_->GetList();
  auto fl_it=fl.begin(),fl_end=fl.end();
  auto efl_it=efl.begin(),efl_end=efl.end();
  for(;fl_it!=fl_end&&efl_it!=efl_end;++fl_it,++efl_it)
  {
    if((*fl_it)->name_->Name()!=(*efl_it)->name_->Name())
      errormsg->Error(this->pos_,"same type required");
    type::Ty* efl_type=(*efl_it)->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty* fl_type=(*fl_it)->ty_;
    if(efl_type&&fl_type&&!fl_type->IsSameType(efl_type))
    {
      errormsg->Error(this->pos_,"fun:%s call:%s",typeid(*fl_type).name(),typeid(*efl_type).name());
      errormsg->Error(this->pos_,"same type required");
    }
  }
  if(fl_it!=fl_end||efl_it!=efl_end)
    errormsg->Error(this->pos_,"record element num error fl:%d efl:%d",fl.size(),efl.size());
    //todo return what?
  return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp*> el = this->seq_->GetList();
  type::Ty* last_ty=nullptr;
  for(const auto& e:el)
  {
    last_ty=e->SemAnalyze(venv,tenv,labelcount,errormsg);
  }
  return last_ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* vty = this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty* ety = this->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);

  // cannot assign to readonly id
  if(venv->IsWithinScope(sym::scope_::FOR_SCOPE))
  {
    if(this->var_&&typeid(*(this->var_))==typeid(absyn::SimpleVar))
    {
      absyn::SimpleVar* sv=static_cast<absyn::SimpleVar*>(this->var_);
      env::EnvEntry* sv_en = venv->LookScope(sv->sym_);
      if(sv_en->readonly_)
      {
        errormsg->Error(this->pos_,"loop variable can't be assigned");
      }
    }
  }
  
  if(vty&&ety&&!vty->IsSameType(ety))
    errormsg->Error(this->pos_,"unmatched assign exp");
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* test_ty=this->test_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(test_ty&&!type::IntTy::Instance()->IsSameType(test_ty))
    errormsg->Error(this->test_->pos_,"integer required!");
  if(this->elsee_==nullptr)
  {
    type::Ty* then_ty=this->then_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(then_ty&&!type::VoidTy::Instance()->IsSameType(then_ty))
      errormsg->Error(this->then_->pos_,"if-then exp's body must produce no value");
    return type::VoidTy::Instance();
  }
  else
  {
    type::Ty* tt=this->then_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty* et=this->elsee_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(tt&&et&&tt->IsSameType(et))
    {
      return tt;
    }
    else{
      errormsg->Error(this->then_->pos_,"then exp and else exp type mismatch");
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope(sym::scope_::WHILE_SCOPE);
  tenv->BeginScope(sym::scope_::WHILE_SCOPE);
  type::Ty* test_ty=this->test_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(test_ty&&!type::IntTy::Instance()->IsSameType(test_ty))
    errormsg->Error(this->test_->pos_,"integer required!");
  type::Ty* body_ty=this->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(body_ty&&!type::VoidTy::Instance()->IsSameType(body_ty))
    errormsg->Error(this->body_->pos_,"while body must produce no value");
  venv->EndScope();
  tenv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope(sym::scope_::FOR_SCOPE);
  tenv->BeginScope(sym::scope_::FOR_SCOPE);
 
  type::Ty * lt=this->lo_->SemAnalyze(venv,tenv,labelcount,errormsg);
  venv->Enter(this->var_,new env::VarEntry(lt,true));
  type::Ty * ht=this->hi_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(ht!=nullptr&&typeid(*ht)!=typeid(type::IntTy))
    errormsg->Error(this->pos_,"for exp's range type is not integer");
  type::Ty* body_ty=this->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(body_ty&&!type::VoidTy::Instance()->IsSameType(body_ty))
    errormsg->Error(this->body_->pos_,"body must be no type");

  venv->EndScope();
  tenv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(!venv->IsWithinScope(sym::scope_::LOOP_SCOPE))
  {
    errormsg->Error(this->pos_,"break is not inside any loop");
  }
  return type::VoidTy::Instance();
}


type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope(sym::scope_::PLAIN_SCOPE);
  tenv->BeginScope(sym::scope_::PLAIN_SCOPE);
  std::list<Dec *> dl=this->decs_->GetList();
  for(const auto& d:dl)
  {
    d->SemAnalyze(venv,tenv,labelcount,errormsg);
  }
  type::Ty* result;
  if(!this->body_) result=type::VoidTy::Instance();
  else result = this->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
  venv->EndScope();
  tenv->EndScope();
  return result;
  
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  type::Ty* tt=tenv->LookScope(this->typ_);
  if(tt==nullptr||typeid(*tt)!=typeid(type::ArrayTy))
  {
    errormsg->Error(this->pos_,"type not exist");
    return tt;
  }
  type::Ty* st=this->size_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty* it=this->init_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(typeid(*st)!=typeid(type::IntTy))
  {
    errormsg->Error(this->pos_,"array size must be int");
  }

  type::ArrayTy* at_it=static_cast<type::ArrayTy*>(tt);
  if(typeid(*it)!=typeid(*(at_it->ty_)))
  {
    errormsg->Error(this->pos_,"type mismatch");
  }
  return tt;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec*> fl=this->functions_->GetList();
  
  for(const auto& function:fl)
  {
    //check name is same
    if(nullptr!=venv->LookScope(function->name_,sym::look_::EXACT_THIS))
    {
      errormsg->Error(this->pos_,"two functions have the same name");
    }

    type::TyList* ftl=new type::TyList();
    std::list<absyn::Field*> param_list = function->params_->GetList();
    for(const auto& field:param_list)
    {
      type::Ty* ty=tenv->LookScope(field->typ_);
      if(ty==nullptr)
      {
        errormsg->Error(this->pos_,"type not exist");
      }
      ftl->Append(ty);
    }
    type::Ty* result_type=type::VoidTy::Instance();
    if(function->result_) result_type = tenv->LookScope(function->result_);
    venv->Enter(function->name_,new env::FunEntry(ftl,result_type));
  }

  for(const auto& function:fl)
  {
    //function is in origin scope
    type::Ty* result_ty=type::VoidTy::Instance();
    if(function->result_) result_ty = tenv->LookScope(function->result_);
    type::TyList* formals=function->params_->MakeFormalTyList(tenv,errormsg);
    venv->Set(function->name_,new env::FunEntry(formals,result_ty));
    auto formal_it=formals->GetList().begin();
    auto param_it=function->params_->GetList().begin();
    auto param_end=function->params_->GetList().end();

    //function in new scope
    venv->BeginScope(sym::scope_::PLAIN_SCOPE);
    for(;param_it!=param_end;formal_it++,param_it++)
    {
      venv->Enter((*param_it)->name_,new env::VarEntry(*formal_it));
    }
    type::Ty* ty=function->body_->SemAnalyze(venv,tenv,labelcount,errormsg);

    //check whether procedure returns value
    if(function->result_==nullptr&&ty&&typeid(*ty)!=typeid(type::VoidTy))
    {
      errormsg->Error(this->pos_,"procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* ty=this->init_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type ty");
  }
  if(this->typ_)
  {
    type::Ty* t = tenv->LookScope(this->typ_);
    if(t&&ty&&!t->IsSameType(ty))
      errormsg->Error(this->pos_,"type mismatch");
  }
  else
  {
    //type is null
    if(ty&&typeid(*ty)==typeid(type::NilTy))
    {
      errormsg->Error(this->pos_,"init should not be nil without type specified");
    }
  }
  //check var name is same
  if(nullptr!=venv->LookScope(this->var_,sym::look_::EXACT_THIS))
  {
    errormsg->Error(this->pos_,"two vars have the same name");
  }
  venv->Enter(this->var_, new env::VarEntry(ty));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> ntl=this->types_->GetList();
  //put the head into tenv
  for(const auto& nt:ntl)
  {
    if(nullptr!=tenv->LookScope(nt->name_,sym::look_::EXACT_THIS))
    {
      errormsg->Error(this->pos_,"two types have the same name");
    }

    tenv->Enter(nt->name_,new type::NameTy(nt->name_,nullptr));
    
  }
  for(const auto& nt:ntl)
  {
    tenv->Set(nt->name_,nt->ty_->SemAnalyze(tenv,errormsg));
    //detect illegel cycle: fast pointer and slow pointer
    type::Ty* head=tenv->LookScope(nt->name_);
    type::Ty* fast=head,*slow=head;


    while(fast!=nullptr&&
    typeid(*fast)==typeid(type::NameTy)&&
    tenv->LookScope(static_cast<type::NameTy*>(fast)->sym_)!=nullptr&&
    typeid(*(tenv->LookScope(static_cast<type::NameTy*>(fast)->sym_)))==typeid(type::NameTy))
    {
      slow=tenv->LookScope(static_cast<type::NameTy*>(slow)->sym_);
      type::Ty* fast_next=tenv->LookScope(static_cast<type::NameTy*>(fast)->sym_);
      fast=tenv->LookScope(static_cast<type::NameTy*>(fast_next)->sym_);
      if(slow==fast)
      {
        errormsg->Error(this->pos_,"illegal type cycle");
        return;
      }
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* ty = tenv->LookScope(this->name_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"undefined type %s",this->name_);
  }
  return new type::NameTy(this->name_,ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::FieldList* fll=new type::FieldList();
  type::RecordTy* record_ty=new type::RecordTy(fll);
  std::list<absyn::Field *> fl=this->record_->GetList();
 
  for(const auto& f:fl)
  {
    type::Ty* ty = tenv->LookScope(f->typ_);

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

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* ty = tenv->LookScope(this->array_);
  if(ty==nullptr)
  {
    errormsg->Error(this->pos_,"no such type");
  }
  return new type::ArrayTy(ty->ActualTy());
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
