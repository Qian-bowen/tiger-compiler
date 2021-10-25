#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int max1=stm1->MaxArgs();
  int max2=stm2->MaxArgs();
  return (max1>max2)?max1:max2;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  Table* t1=stm1->Interp(t);
  return stm2->Interp(t1);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable it=exp->InterExp(t);
  return it.t->Update(id,it.i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  std::list<Exp*> all_exp;
  exps->GetAllExps(all_exp);
  int tmax=all_exp.size();
  for(auto& exp:all_exp)
  {
    int ctmp=exp->MaxArgs();
    if(ctmp>tmax) tmax=ctmp;
  }
  return tmax;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  Table* tmp=t;
  std::list<Exp*> exp_list;
  exps->GetAllExps(exp_list);
  for(auto& exp:exp_list)
  {
    IntAndTable it=exp->InterExp(tmp);
    tmp=it.t;
    std::cout<<it.i<<' ';
  }
  std::cout<<std::endl;
  return tmp;
}

IntAndTable IdExp::InterExp(Table * t) const{
  return IntAndTable(t->Lookup(id),t);
}

IntAndTable NumExp::InterExp(Table * t) const{
  return IntAndTable(num,t);
}

int OpExp::MaxArgs() const{
  int lmax=left->MaxArgs();
  int rmax=right->MaxArgs();
  return (lmax>=rmax)?lmax:rmax;
}

IntAndTable OpExp::InterExp(Table * t) const{
  IntAndTable lval=left->InterExp(t);
  IntAndTable rval=right->InterExp(lval.t);
  int operi=0;
  switch(oper){
    case PLUS:
      operi = lval.i+rval.i;
      break;
    case MINUS:
      operi = lval.i-rval.i;
      break;
    case TIMES:
      operi = lval.i*rval.i;
      break;
    case DIV:
      operi = lval.i/rval.i;
      break;
    default:
      assert(false);
  }
  return IntAndTable(operi,rval.t);
}

int EseqExp::MaxArgs() const{
  int smax=stm->MaxArgs();
  int emax=exp->MaxArgs();
  return (smax>=emax)?smax:emax;
}

IntAndTable EseqExp::InterExp(Table * t) const{
  stm->Interp(t);
  return exp->InterExp(t);
}

void PairExpList::GetAllExps(std::list<Exp*>& exp_list)const{
  tail->GetAllExps(exp_list);
  exp_list.push_front(exp);
}

void LastExpList::GetAllExps(std::list<Exp*>& exp_list)const{
  exp_list.push_back(exp);
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
