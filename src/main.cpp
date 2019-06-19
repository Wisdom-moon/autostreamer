//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "PassManager.h"


static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

ScopeIR * TopScope;
SourceManager *SM;
ScopeIR * CurScope;
ASTContext *Ctx;

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  TopScope = new ScopeIR();
  TopScope->type = -1;

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  int ret = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  delete TopScope;
  return ret;
}
