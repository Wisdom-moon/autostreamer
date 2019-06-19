//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#ifndef PASSMANAGER_H
#define PASSMANAGER_H

#include "KernelExtractor.h"
#include "CodeGen.h"
#include "ScopeIRGen.h"
#include "FunctionInfo.h"
#include "LoopInfo.h"

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
MyASTConsumer(std::vector<struct Kernel_Info> &k_info_queue, std::vector<struct Scope_data> &s_stack) : Visitor(k_info_queue, s_stack), Scope_stack(s_stack) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override;

  virtual void Initialize(ASTContext &Context) {
    Ctx = &Context;
    Visitor.Initialize (Context);
    scopeGen.Initialize();
  }

private:
  KernelExtractor Visitor;
  ScopeIRGen scopeGen;
  std::vector<struct Scope_data> &Scope_stack;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
private:
  struct Kernel_Info * select_kernel ();
public:
  MyFrontendAction() {}

//Callback at the end of processing a single input.
  void EndSourceFileAction() override;

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override;

private:
  CodeGen generator;
  std::vector<struct Kernel_Info> k_info_queue;
  std::vector<struct Scope_data> Scope_stack;
};
 
#endif
