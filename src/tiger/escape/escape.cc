#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  root_->Traverse(env,0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry * ee = env->Look(this->sym_);
  if(ee==nullptr) return;
  if(depth>ee->depth_)
  {
    *(ee->escape_)=true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
  this->subscript_->Traverse(env,depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> exp_list = this->args_->GetList();
  for(const auto& exp:exp_list)
  {
    exp->Traverse(env,depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env,depth);
  this->right_->Traverse(env,depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> exp_list = this->seq_->GetList();
  for(const auto& exp:exp_list)
  {
    exp->Traverse(env,depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
  this->exp_->Traverse(env,depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env,depth);
  this->then_->Traverse(env,depth);
  if(this->elsee_) this->elsee_->Traverse(env,depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env,depth);
  this->body_->Traverse(env,depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_=false;
  env->Enter(this->var_, new esc::EscapeEntry(depth, &(this->escape_)));
  this->lo_->Traverse(env,depth);
  this->hi_->Traverse(env,depth);
  this->body_->Traverse(env,depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<absyn::Dec *> dec_list = this->decs_->GetList();
  for(auto& dec:dec_list)
  {
    dec->Traverse(env,depth);
  }
  body_->Traverse(env,depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  size_->Traverse(env,depth);
  init_->Traverse(env,depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  ++depth;
  std::list<absyn::FunDec *> func_list = this->functions_->GetList();
  for(auto& dec:func_list)
  {
    env->BeginScope(sym::PLAIN_SCOPE);
    std::list<absyn::Field*> param=dec->params_->GetList();
    for(auto& p:param)
    {
      // todo error!
      p->escape_=false;
      //do not check param escape
      env->Enter(p->name_,new esc::EscapeEntry(depth,&(p->escape_)));
    }
    dec->body_->Traverse(env,depth);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_=false;
  env->Enter(this->var_,new esc::EscapeEntry(depth,&(this->escape_)));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

} // namespace absyn
