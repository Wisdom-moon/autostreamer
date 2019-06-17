//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "ScopeIR.h"


class ScopeIRGen: public RecursiveASTVisitor<ScopeIRGen> {
  private:
  //TODO:If the *ptr is calculated form one base ptr, we may trace it.
  Access_Var * create_av_expr(Expr * e) {
    Access_Var *av = NULL;
    if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(e)) {
      av = new Access_Var(ref);
    }
    else if (ArraySubscriptExpr * ArrEx = dyn_cast<ArraySubscriptExpr>(e)) {
      av = new Access_Var(ArrEx);
      Expr *base = ArrEx->getBase()->IgnoreImpCasts();
      TraverseStmt(ArrEx->getIdx()->IgnoreImpCasts());
      while (isa<ArraySubscriptExpr>(base)) {
        ArraySubsciptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
        TraverseStmt(a->getIdx()->IgnoreImpCasts());
        base = a->getBase()->IgnoreImpCasts();
      }
    }
    else if ((UnaryOperator *UnOp = dyn_cast<UnaryOperator>(e)) &&
  	    UnOp->getOpcode() == UO_Deref){
      Expr * sub_e = UnOo->getSubExpr()->IgnoreImpCasts();
      if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(e)) {
        av = new Access_Var(ref);
    
        //Create IntegerLiteral expression "0".
        llvm::APInt int_val(64, 0, true);
        QualType type = Ctx->getIntTypeForBitwidth(64, true);
        IntegerLiteral *int_expr = new (*Ctx) IntegerLiteral(*Ctx, int_val, type, e->getLocation());
    
        av->set_index(int_expr);
      }
      else {
        UnOp->dump();
        cerr <<"Exception Deref Mem to create Access_Var!\n";
      }
    }
    else {
      UnOp->dump();
      cerr <<"Exception Expr to create Access_Var!\n";
    }
  
    return av;
  }

  public:
  void Initialize(ASTContext &Context) {
    CurScope = new ScopeIR();
    CurScope->type = 0;
    TopScope->add_child(CurScope);
  }

  bool VisitFunctionDecl (FunctionDecl *f) {
    if (f->doesThisDeclarationHaveABody()) {
      Decl_Var * f_var = new Decl_Var(f);
      CurScope->append_decl(f_var);

      ScopeIR * p_scope = new ScopeIR();
      p_scope->type = 1;
      p_scope->function = f_var;
      CurScope->add_child(p_scope);
      CurScope = p_scope;
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *d) {
    Decl_var * p_var = new Decl_Var(d);
    CurScope->append_decl (p_var);
    return true;
  }

  bool VisitStmt(Stmt *S) {
    if (ReturnStmt * ret = dyn_cast<ReturnStmt> (S)) {
      ScopeIR * func = this;
      int line = SM->getExpansionLineNumber(ret->getLocation());
      while (func && func->type != 1) func = func->parent;

      if (func && func->last_return_line < line) 
        func->last_return_line = line; 
    }
 
    return true;
  }

  bool dataTraverseStmtPost (Stmt *st) {
    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
	if (CurScope->parent->type == 1)
	  CurScope = CurScope->parent;
      case clang::Stmt::ForStmtClass: ;
      case clang::Stmt::IfStmtClass: ;
      case clang::Stmt::WhileStmtClass: ;
      case clang::Stmt::DoStmtClass: ;
      case clang::Stmt::SwitchStmtClass: ;
      case clang::Stmt::SwitchCaseClass: ;
      case clang::Stmt::OMPParallelForDirectiveClass: ;
      case clang::Stmt::CXXCatchStmtClass: ;
      case clang::Stmt::CXXForRangeStmtClass: ;
      case clang::Stmt::CXXTryStmtClass: ;
      case clang::Stmt::SEHExceptStmtClass: ;
      case clang::Stmt::SEHFinallyStmtClass: ;
      case clang::Stmt::CapturedStmtClass: ;
      case clang::Stmt::NoStmtClass:
	CurScope = CurScope->parent;
	break;
      default:
	break;
    }
  return true;
  }

  bool dataTraverseStmtPre (Stmt *st) {
    ScopeIR * p_scope = new ScopeIR();
    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
	p_scope->type = 2; break;
      case clang::Stmt::ForStmtClass: ;
	p_scope->type = 3;
	p_scope->condition = st;
	break;
      case clang::Stmt::IfStmtClass: ;
	p_scope->type = 4; 
	p_scope->condition = st;
	break;
      case clang::Stmt::WhileStmtClass: ;
	p_scope->type = 5; 
	p_scope->condition = st;
	break;
      case clang::Stmt::DoStmtClass: ;
	p_scope->type = 6; 
	p_scope->condition = st;
	break;
      case clang::Stmt::SwitchStmtClass: ;
	p_scope->type = 7; 
	p_scope->condition = st;
	break;
      case clang::Stmt::SwitchCaseClass: ;
	p_scope->type = 8; 
	p_scope->condition = st;
	break;
      case clang::Stmt::OMPParallelForDirectiveClass: ;
	p_scope->type = 9; break;
      case clang::Stmt::CXXCatchStmtClass: ;
	p_scope->type = 10; break;
      case clang::Stmt::CXXForRangeStmtClass: ;
	p_scope->type = 11; break;
      case clang::Stmt::CXXTryStmtClass: ;
	p_scope->type = 12; break;
      case clang::Stmt::SEHExceptStmtClass: ;
	p_scope->type = 13; break;
      case clang::Stmt::SEHFinallyStmtClass: ;
	p_scope->type = 14; break;
      case clang::Stmt::CapturedStmtClass: ;
	p_scope->type = 15; break;
      default:
	delete p_scope;
	p_scope = NULL;
	break;
    }

  if (p_scope != NULL) {
    p_scope->s_pos = new Position(st->getLocStart());
    p_scope->e_pos = new Position(st->getLocEnd());
    CurScope->add_child(p_scope);
    CurScope = p_scope;
  }

  return true;
  }

  //Construct Use-Def Chains.
  //And add scope for branch statements.
  bool TraverseStmt(Stmt *S) {
    if (!S)
      return true;
  
    switch (S->getStmtClass()) {
      case clang::Stmt::BinaryOperator:
        BinaryOperator *BinOp = dyn_cast<BinaryOperator>(S) 
        //Handle all Assign write operation here.
        switch (BinOp->getOpcode() ) {
          case BO_Assign:
            Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
            Access_Var * av = create_av_expr (lhs);
            if (av) {
              av->set_value (BinOp->getRHS()->IgnoreImpCasts());
              CurScope->addUDChain(av);
              return TraverseStmt(BinOp->getRHS());
            }
            break;
        }
	break;
      case clang::Stmt::CompoundAssignOperator:
        CompoundAssignOperator *BinOp = dyn_cast<CompoundAssignOperator>(S) 
        switch (BinOp->getOpcode() ) {
          case BO_MulAssign:
          case BO_DivAssign:
          case BO_AddAssign:
          case BO_SubAssign:
            Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
            Access_Var * av = create_av_expr (lhs);
            if (av) {
              av->set_value (BinOp);
              CurScope->addUDChain(av);
              return TraverseStmt(BinOp->getRHS());
            }
            break;
        }
      case clang::Stmt::UnaryOperator:
        UnaryOperator *UnOp = dyn_cast<UnaryOperator>(S)
        switch (UnOp->getOpcode()) {
          //Handle all unary write operation here.
          case UO_PostInc:
          case UO_PostDes:
          case UO_PreInc:
          case UO_PreDec:
            Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
            Access_Var * av = create_av_expr (e);
            if (av) {
              av->set_value (UnOp);
              CurScope->addUDChain(av);
              return true;
            }
            break;

          // *ptr, read;
          // TODO: *++ptr, *(exp), *arr[], 
          case UO_Deref:
            Access_Var * av = create_av_expr(UnOp);
            if (av) {
              CurScope->addUDChain(av);
              return true;
            }
            break;
          case UO_AddrOf:
          case UO_Plus:
          case UO_Minus:
            break;
        }
        break;
      //TODO: Struct member read.
      case MemberExpr:
        MemberExpr * MemEx = dyn_cast<MemberExpr>(S) 
        return RecursiveASTVisitor<MyASTVisitor>::TraverseStmt(S);
        break;
      //Array[][] Read.
      case ArraySubscriptExpr:
      case DeclRefExpr:
        Access_Var * av = create_av_expr (S);
        if (av) {
          CurScope->addUDChain(av);
          return true;
        }
        break;
      //Create scope of true/false branch.
      case IfStmt:
	IfStmt * ifs = cast<IfStmt>(S);
	Stmt * e = ifs->getCond();
  	TraverseStmt(e);
	e = ifs->getInit();
  	TraverseStmt(e);

	e = ifs->getThen();
	if (e) {
          ScopeIR * true_scope = new ScopeIR();
	  true_scope->type = 15;
          true_scope->s_pos = new Position(e->getLocStart());
          true_scope->e_pos = new Position(e->getLocEnd());
          CurScope->add_child(true_scope);
          CurScope = true_scope;
	  TraverseStmt(e);
	  CurScope = CurScope->parent;
	}

	e = ifs->getElse();
	if (e) {
          ScopeIR * false_scope = new ScopeIR();
	  true_scope->type = 16;
          false_scope->s_pos = new Position(e->getLocStart());
          false_scope->e_pos = new Position(e->getLocEnd());
          CurScope->add_child(false_scope);
          CurScope = false_scope;
  	  TraverseStmt(e);
  	  CurScope = CurScope->parent;
	}

	return true;
      default:
	break;
    }

    return RecursiveASTVisitor<MyASTVisitor>::TraverseStmt(S);
  }

}
