//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {

class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  temp::Temp *rax, *rbx, *rcx, *rdx, *rsi, *rdi, *rbp, *rsp, *r8, *r9, *r10,
      *r11, *r12, *r13, *r14, *r15;

public:
  X64RegManager();
  virtual ~X64RegManager() = default;

  virtual temp::TempList *Registers() override {
    return new temp::TempList({rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, r9,
                               r10, r11, r12, r13, r14, r15});
  }
  virtual temp::TempList *ArgRegs() override {
    return new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
  }
  virtual temp::TempList *CallerSaves() override {
    return new temp::TempList({rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11});
  } // except rsp and callee save reg
  virtual temp::TempList *CalleeSaves() override {
    return new temp::TempList({rbx, rbp, r12, r13, r14, r15});
  }
  virtual temp::TempList *ReturnSink() override {
    return new temp::TempList({rsp, rax, rbx, rbp, r12, r13, r14, r15});
  }
  virtual int WordSize() override { return 8; } // eight bytes 64 bit address
  virtual temp::Temp *FramePointer() override { return rbp; }
  virtual temp::Temp *StackPointer() override { return rsp; }
  virtual temp::Temp *ReturnValue() override { return rax; }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
