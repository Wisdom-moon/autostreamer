//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "ScopeIRGen.h"
#include <iostream>


//TODO:If the *ptr is calculated form one base ptr, we may trace it.
Access_Var * ScopeIRGen::create_av_expr(Expr * e) {
  Access_Var *av = NULL;
  if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(e)) {
    av = new Access_Var(ref);
  }
  else if (ArraySubscriptExpr * ArrEx = dyn_cast<ArraySubscriptExpr>(e)) {
    av = new Access_Var(ArrEx);
    Expr *base = ArrEx->getBase()->IgnoreImpCasts();
    TraverseStmt(ArrEx->getIdx()->IgnoreImpCasts());
    while (isa<ArraySubscriptExpr>(base)) {
      ArraySubscriptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
      TraverseStmt(a->getIdx()->IgnoreImpCasts());
      base = a->getBase()->IgnoreImpCasts();
    }
  }
  else if (UnaryOperator *UnOp = dyn_cast<UnaryOperator>(e)){
    if (UnOp->getOpcode() == UO_Deref){
      Expr * sub_e = UnOp->getSubExpr()->IgnoreImpCasts();
      if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(sub_e)) {
        av = new Access_Var(ref);
  
        //Create IntegerLiteral expression "0".
        llvm::APInt int_val(64, 0, true);
        QualType type = Ctx->getIntTypeForBitwidth(64, true);
        IntegerLiteral *int_expr = new (*Ctx) IntegerLiteral(*Ctx, int_val, type, UnOp->getLocStart());
  
        av->set_index(int_expr);
      }
      else {
        UnOp->dump();
        std::cerr <<"Exception Deref Mem to create Access_Var!\n";
      }
    }
  }
  else if (isa<IntegerLiteral>(e))
    return NULL;

  if (av == NULL) {
    e->dump();
    std::cerr <<"Exception Expr to create Access_Var!\n";
  }

  return av;
}


  bool ScopeIRGen::VisitFunctionDecl (FunctionDecl *f) {
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

  bool ScopeIRGen::VisitVarDecl(VarDecl *d) {
    Decl_Var * p_var = new Decl_Var(d);
    CurScope->append_decl (p_var);
    if (d->hasInit()) {
      p_var->init_value = new ValueRange(d->getInit()->IgnoreImpCasts());

      Access_Var * av = new Access_Var(d);
      av->set_value(d->getInit()->IgnoreImpCasts());
      CurScope->addUDChain(av);
    }
    return true;
  }

  bool ScopeIRGen::VisitStmt(Stmt *S) {
    if (ReturnStmt * ret = dyn_cast<ReturnStmt> (S)) {
      ScopeIR * func = CurScope;
      int line = SM->getExpansionLineNumber(ret->getLocStart());
      while (func && func->type != 1) func = func->parent;

      if (func && func->last_return_line < line) 
        func->last_return_line = line; 
    }
    else if (CallExpr * call_e = dyn_cast<CallExpr> (S))
      CurScope->callers.push_back(call_e);
 
    return true;
  }

  bool ScopeIRGen::dataTraverseStmtPost (Stmt *st) {
    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
	if (CurScope->parent->type == 1)
	  CurScope = CurScope->parent;
	CurScope = CurScope->parent;
	break;
      case clang::Stmt::ForStmtClass: 
      case clang::Stmt::IfStmtClass: 
      case clang::Stmt::WhileStmtClass: 
      case clang::Stmt::DoStmtClass: 
      case clang::Stmt::SwitchStmtClass:
      case clang::Stmt::OMPParallelForDirectiveClass: 
      case clang::Stmt::CXXCatchStmtClass: 
      case clang::Stmt::CXXForRangeStmtClass: 
      case clang::Stmt::CXXTryStmtClass: 
      case clang::Stmt::SEHExceptStmtClass: 
      case clang::Stmt::SEHFinallyStmtClass: 
      case clang::Stmt::CapturedStmtClass: 
      case clang::Stmt::NoStmtClass:
	CurScope = CurScope->parent;
	break;
      default:
	break;
    }
  return true;
  }

  //0: One file scope; 1: function; 2: for loop; 3: do loop; 4: while loop;
  //5: if stmt; 6: switch stmt; 7: switch case;
  //8: Try; 9: Catch; 10: SEH; 11: compound; 12: others.
  //13: omp parallel for; 14: captured stmt;
  //15: if then part; 16: if else part;
  bool ScopeIRGen::dataTraverseStmtPre (Stmt *st) {
    ScopeIR * p_scope = new ScopeIR();
    p_scope->condition = st;
    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
	p_scope->type = 11; 
	break;
      case clang::Stmt::ForStmtClass: ;
	p_scope->type = 2;
	break;
      case clang::Stmt::IfStmtClass: ;
	p_scope->type = 5; 
	break;
      case clang::Stmt::WhileStmtClass: ;
	p_scope->type = 4; 
	break;
      case clang::Stmt::DoStmtClass: ;
	p_scope->type = 3; 
	break;
      case clang::Stmt::SwitchStmtClass: ;
	p_scope->type = 6; 
	break;
//      case clang::Stmt::SwitchCaseClass: ;
//	p_scope->type = 8; 
//	p_scope->condition = st;
//	break;
      case clang::Stmt::OMPParallelForDirectiveClass: ;
	p_scope->type = 13; break;
      case clang::Stmt::CXXCatchStmtClass: ;
	p_scope->type = 9; break;
      case clang::Stmt::CXXForRangeStmtClass: ;
	p_scope->type = 12; break;
      case clang::Stmt::CXXTryStmtClass: ;
	p_scope->type = 8; break;
      case clang::Stmt::SEHExceptStmtClass: ;
	p_scope->type = 10; break;
      case clang::Stmt::SEHFinallyStmtClass: ;
	p_scope->type = 12; break;
      case clang::Stmt::CapturedStmtClass: ;
	p_scope->type = 14; break;
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
  bool ScopeIRGen::TraverseStmt(Stmt *S) {
    if (!S)
      return true;
  
    switch (S->getStmtClass()) {
      case clang::Stmt::BinaryOperatorClass:
	{
          BinaryOperator *BinOp = dyn_cast<BinaryOperator>(S); 
          //Handle all Assign write operation here.
          if (BinOp->getOpcode() == BO_Assign) {
            Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
            Access_Var * av = create_av_expr (lhs);
            if (av) {
              av->set_value (BinOp->getRHS()->IgnoreImpCasts());
              CurScope->addUDChain(av);
              return TraverseStmt(BinOp->getRHS());
            }
          }
	}
	break;
      case clang::Stmt::CompoundAssignOperatorClass:
	{
          CompoundAssignOperator *BinOp = dyn_cast<CompoundAssignOperator>(S);
          switch (BinOp->getOpcode()) {
            case BO_MulAssign:
            case BO_DivAssign:
            case BO_AddAssign:
            case BO_SubAssign:
	      {
                Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
                Access_Var * av = create_av_expr (lhs);
                if (av) {
                  av->set_value (BinOp);
                  CurScope->addUDChain(av);
                  return TraverseStmt(BinOp->getRHS());
                }
	      }
              break;
	    default:
	      break;
          }
	}
	break;
      case clang::Stmt::UnaryOperatorClass:
	{
          UnaryOperator *UnOp = dyn_cast<UnaryOperator>(S);
          switch (UnOp->getOpcode()) {
            //Handle all unary write operation here.
            case UO_PostInc:
            case UO_PostDec:
            case UO_PreInc:
            case UO_PreDec:
	      {
                Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
                Access_Var * av = create_av_expr (e);
                if (av) {
                  av->set_value (UnOp);
                  CurScope->addUDChain(av);
                  return true;
                }
	      }
              break;

            // *ptr, read;
            // TODO: *++ptr, *(exp), *arr[], 
            case UO_Deref:
	      {
                Access_Var * av = create_av_expr(UnOp);
                if (av) {
                  CurScope->addUDChain(av);
                  return true;
                }
	      }
              break;
            case UO_AddrOf:
            case UO_Plus:
            case UO_Minus:
              break;
	    default:
	      break;
          }
	}
        break;
      //TODO: Struct member read.
      case Stmt::MemberExprClass:
	{
          //MemberExpr * MemEx = dyn_cast<MemberExpr>(S); 
          return RecursiveASTVisitor<ScopeIRGen>::TraverseStmt(S);
	}
        break;
      //Array[][] Read.
      case Stmt::ArraySubscriptExprClass:
      case Stmt::DeclRefExprClass:
	{
          Access_Var * av = create_av_expr (cast<Expr>(S));
          if (av) {
            CurScope->addUDChain(av);
            return true;
          }
	}
        break;
      //Create scope of true/false branch.
      case Stmt::IfStmtClass:
	{
	  IfStmt * ifs = cast<IfStmt>(S);
	  Stmt * e = ifs->getCond();
  	  TraverseStmt(e);
	  e = ifs->getInit();
  	  TraverseStmt(e);

	  e = ifs->getThen();
	  if (e) {
            ScopeIR * true_scope = new ScopeIR();
	    true_scope->condition = e;
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
	    false_scope->condition = e;
	    false_scope->type = 16;
            false_scope->s_pos = new Position(e->getLocStart());
            false_scope->e_pos = new Position(e->getLocEnd());
            CurScope->add_child(false_scope);
            CurScope = false_scope;
  	    TraverseStmt(e);
  	    CurScope = CurScope->parent;
	  }
	}
	return true;
      default:
	break;
    }

    return RecursiveASTVisitor<ScopeIRGen>::TraverseStmt(S);
  }
