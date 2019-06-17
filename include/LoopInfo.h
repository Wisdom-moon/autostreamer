//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef LOOPINFO_H
#define LOOPINFO_H

#include "ScopeIR.h"

class LoopVar {
  private:
  Decl_Var * var_decl;
  // For these flags, 0 means undefine, -1 means false, 1 means true;
  int inductive;

  //TODO: wait for further dependence analysis need.
  //std::vector<std::pair<Access_Var *, Access_Var *>> dependenceMap;
  std::vector<Access_Var *> accessInst;

  public:
  LoopVar() {var_decl = NULL;inductive = 0;}
  ~LoopVar() {}
  bool tryMerge(LoopVar * lv);
  void addAV(Access_Var *);
}

class LoopInfo {
  private:

  std::vector<LoopVar *> loopVars;
  bool hasCollected;

  // For these flags, 0 means undefine, -1 means false, 1 means true;
  int parallelizable; //The for loop can be parallel or not.
  int perfect; //The for loop is perfect or not.
  int canonical;//normal For loop, such as for (; var OP expr; step(var))

  
  Decl_Var * iter;

  LoopInfo * createInnerLoop();
  std::vector<LoopInfo *> children;
  void collectVars();
  LoopVar * addVar(Access_Var *);

  public:
  ScopeIR * scope;

  LoopInfo() {scope=NULL;hasCollected=false;parallelizable=0;perfect=0;canonical=0;}
  LoopInfo(ScopeIR * s);
  ~LoopInfo() {}
  bool isPerfect();
  bool isParallelizable();
  bool isCanonical();
  bool isInductive(LoopVar *v);

  void buildLoopTree();

  void genKernelInfo(Kernel_Info &ki);
}

#endif
