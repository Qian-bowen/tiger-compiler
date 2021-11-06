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
enum scope_{PLAIN_SCOPE,FOR_SCOPE,WHILE_SCOPE,LOOP_SCOPE};

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
  // sym::Symbol* getLoopSymbol(){return &markloop_;}

private:
  Symbol marksym_ = {"<mark>", nullptr};
  
  Symbol markloop_ = {"<loop>", nullptr};
  Symbol markfor_ = {"<for>", nullptr};
  Symbol markwhile_ = {"<while>", nullptr};
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
}

template <typename ValueType> void Table<ValueType>::EndScope() {
  Symbol *s;
  do
    s = this->Pop();
  while (s != &marksym_);
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
  return flag;
}

} // namespace sym

#endif // TIGER_SYMBOL_SYMBOL_H_
