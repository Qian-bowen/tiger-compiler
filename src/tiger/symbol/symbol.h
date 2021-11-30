#ifndef TIGER_SYMBOL_SYMBOL_H_
#define TIGER_SYMBOL_SYMBOL_H_

#include <string>

#include "tiger/util/table.h"

/**
 * Forward Declarations
 */
namespace env {
class EnvEntry;
} // namespace env
namespace type {
class Ty;
} // namespace type

namespace sym {

//LOOP_SCOPE contains FOR_SCOPE and WHILE_SCOPE
enum scope_{PLAIN_SCOPE,FOR_SCOPE,WHILE_SCOPE,LOOP_SCOPE,FUNCDEC_SCOPE};
enum look_{EXACT_LARGER,EQUAL_LARGER,EXACT_THIS};

class Symbol {
  template <typename ValueType> friend class Table;

public:
  static Symbol *UniqueSymbol(std::string_view);
  [[nodiscard]] std::string Name() const { return name_; }

private:
  Symbol(std::string name, Symbol *next)
      : name_(std::move(name)), next_(next) {}

  std::string name_;
  Symbol *next_;
};

template <typename ValueType>
class Table : public tab::Table<Symbol, ValueType> {
public:
  Table() : tab::Table<Symbol, ValueType>() {}
  void BeginScope(scope_ scope_mark);
  void EndScope();
  bool IsWithinScope(scope_ scope_mark);

  //override parent function
  //to check scope
  void Enter(Symbol *key, ValueType *value)override;
  void Set(Symbol *key, ValueType *value)override;

  ValueType *LookScope(Symbol *key,look_ look=look_::EQUAL_LARGER);

private:
  // int depth_=0;//depth of scope
  Symbol marksym_ = {"<mark>", nullptr};
  
  Symbol markloop_ = {"<loop>", nullptr};
  Symbol markfor_ = {"<for>", nullptr};
  Symbol markwhile_ = {"<while>", nullptr};
  Symbol markfuncdec_ = {"<funcdec>", nullptr};
};

template <typename ValueType> void Table<ValueType>::BeginScope(scope_ scope_mark) {
  this->Enter(&marksym_, nullptr);
  if(scope_mark==scope_::FOR_SCOPE)
  {
    this->Enter(&markloop_, nullptr);
    this->Enter(&markfor_, nullptr);
  }
  else if(scope_mark==scope_::WHILE_SCOPE)
  {
    this->Enter(&markloop_, nullptr);
    this->Enter(&markwhile_, nullptr);
  }
  else if(scope_mark==scope_::LOOP_SCOPE)
  {
    this->Enter(&markloop_, nullptr);
  }
  else if(scope_mark==scope_::FUNCDEC_SCOPE)
  {
    this->Enter(&markfuncdec_, nullptr);
  }
  // this->depth_++;
}

template <typename ValueType> void Table<ValueType>::EndScope() {
  Symbol *s;
  do
    s = this->Pop();
  while (s != &marksym_);
  // this->depth_--;
}

template <typename ValueType> bool Table<ValueType>::IsWithinScope(scope_ scope_mark) {
  bool flag=false;
  if(scope_mark==scope_::FOR_SCOPE)
  {
    flag=this->IsExist(&markfor_);
  }
  else if(scope_mark==scope_::LOOP_SCOPE)
  {
    flag=this->IsExist(&markloop_);
  }
  else if(scope_mark==scope_::WHILE_SCOPE)
  {
    flag=this->IsExist(&markwhile_);
  }
  else if(scope_mark==scope_::FUNCDEC_SCOPE)
  {
    flag=this->IsExist(&markfuncdec_);
  }
  return flag;
}

template <typename ValueType> void Table<ValueType>::Enter(Symbol *key, ValueType *value)
{
  // if(value) value->depth_=this->depth_;
  tab::Table<Symbol, ValueType>::Enter(key,value);
}

template <typename ValueType> void Table<ValueType>::Set(Symbol *key, ValueType *value)
{
  // if(value) value->depth_=this->depth_;
  tab::Table<Symbol, ValueType>::Set(key,value);
}

/**
 * @brief 
 * this function do not check type, just search name in some scope
 * default value of look is EQUAL_LARGERR in declaration
 * for a element in table with depth i, all elements that can be searched have depth<=i, they are all popped
 * since you cannot find depth>i,so there is no need to implement EQUAL_LARGER
 * EXACT_THIS means you just look up in this scope
 * @param key 
 * @param look 
 * @return template <typename ValueType>* 
 */
template <typename ValueType> ValueType* Table<ValueType>::LookScope(Symbol *key,look_ look)
{
  ValueType* vt=tab::Table<Symbol, ValueType>::Look(key);
  // if(vt==nullptr) return nullptr;

  // if(look==look_::EXACT_LARGER&&vt->depth_>=this->depth_)
  // {
  //   return nullptr;
  // }
  // else if(look==look_::EXACT_THIS&&vt->depth_!=this->depth_)
  // {
  //   return nullptr;
  // }
  return vt;
}

} // namespace sym

#endif // TIGER_SYMBOL_SYMBOL_H_
