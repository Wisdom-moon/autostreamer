//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef SCOPEIRGEN_H
#define SCOPEIRGEN_H

#include "ScopeIR.h"

class ScopeIRGen: public RecursiveASTVisitor<ScopeIRGen> {
  private:
  //TODO:If the *ptr is calculated form one base ptr, we may trace it.
  Access_Var * create_av_expr(Expr * e);

  public:
  void Initialize() {
    CurScope = new ScopeIR();
    CurScope->type = 0;
    TopScope->add_child(CurScope);
  }
  bool VisitFunctionDecl (FunctionDecl *f) ;
  bool VisitVarDecl(VarDecl *d) ;
  bool VisitStmt(Stmt *S) ;
  bool dataTraverseStmtPost (Stmt *st) ;
  bool dataTraverseStmtPre (Stmt *st) ;
  bool TraverseStmt(Stmt *S) ;
};

#endif
