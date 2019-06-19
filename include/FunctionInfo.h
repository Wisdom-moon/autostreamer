//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef FUNCTIONINFO_H
#define FUNCTIONINFO_H

#include "ScopeIR.h"
#include "KernelInfo.h"

//TODO:Construct the caller and callee graph.
class FunctionInfo {
  private:

  public:
  LoopInfo *rootLoop;
  ScopeIR * scope;

  FunctionInfo() {rootLoop = NULL; scope = NULL;}
  FunctionInfo(ScopeIR * s) {rootLoop = NULL; scope = s; s->setFunctionInfo(this);}
  ~FunctionInfo() {}

  void genFileInfo(File_Info & fi);
};

#endif
