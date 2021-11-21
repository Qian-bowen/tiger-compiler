//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {


class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  virtual temp::TempList *Registers()override;
  virtual temp::TempList *ArgRegs()override;
  virtual temp::TempList *CallerSaves()override;
  virtual temp::TempList *CalleeSaves()override;
  virtual temp::TempList *ReturnSink()override;
  virtual int WordSize()override;
  virtual temp::Temp *FramePointer()override;
  virtual temp::Temp *StackPointer()override;
  virtual temp::Temp *ReturnValue()override;
};


} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
