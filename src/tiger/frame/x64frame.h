//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {


class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager():RegManager(){}
  virtual ~X64RegManager(){}

  virtual temp::TempList *Registers(){return nullptr;}
  virtual temp::TempList *ArgRegs(){return nullptr;}
  virtual temp::TempList *CallerSaves(){return nullptr;}
  virtual temp::TempList *CalleeSaves(){return nullptr;}
  virtual temp::TempList *ReturnSink(){return nullptr;}
  virtual int WordSize(){return 0;}
  virtual temp::Temp *FramePointer(){return nullptr;}
  virtual temp::Temp *StackPointer(){return nullptr;}
  virtual temp::Temp *ReturnValue(){return nullptr;}
};


} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
