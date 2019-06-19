//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include <iostream>
#include "ScopeIR.h"
#include "LoopInfo.h"
#include "FunctionInfo.h"
#include "Util.h"

// Class Position
Position::Position() {
}

Position::Position(SourceLocation loc) {
  line = SM->getExpansionLineNumber(loc);
  col = SM->getExpansionColumnNumber(loc);
}

Position::~Position() {
}

bool Position::operator<(const Position & pos){
  return (line < pos.line || (line == pos.line && col < pos.col));
}

bool Position::operator>(const Position & pos){
  return (line > pos.line || (line == pos.line && col > pos.col));
}

// Class ValueRange Implementation.
ValueRange::ValueRange() {
}

ValueRange::~ValueRange() {
}

ValueRange::ValueRange(Expr *e) {
  lower_boundary = e;
  lower_offset = 0;
  upper_boundary = e;
  upper_offset = 0;
}

// Class Decl_Var Implementation.
// For each variable declar.
Decl_Var::Decl_Var() {
  init_value = NULL;
  pos = NULL;
}

Decl_Var::Decl_Var(ValueDecl * d) {
  init_value = NULL;
  pos = new Position(d->getLocation());
  name = d->getName().str();
  decl_stmt = d;
  type = d->getType().getAsString();

  switch (d->getKind()) {
    case Decl::ParmVar:
	category = 1;
	break;
    case Decl::Function:
	category = 0;
	break;
    case Decl::Var:
	category = 3;
	break;
    default:
	break;
  }

  if (category > 0 && type.find("*") != std::string::npos)
    category ++;

}

Decl_Var::~Decl_Var() {
    if (init_value != NULL)
      delete init_value;
    if (pos != NULL)
      delete pos;
}

//TODO: We have not consider the loop context;
Access_Var * Decl_Var::find_last_access(Position &pos, bool iswrite){
  for (int i = (int)access_chain.size() - 1; i >= 0; i--) {
    if (*(access_chain[i]->pos) < pos && access_chain[i]->isWrite() == iswrite)
      return access_chain[i];
  }

  return NULL;
}

Access_Var * Decl_Var::find_next_access(Position &pos, bool iswrite){
  for (auto& av : access_chain) {
    if (*(av->pos) > pos && av->isWrite() == iswrite)
      return av;
  }

  return NULL;
}


// Class Access_Var Implementation.
// For each access of one variable.
Access_Var::Access_Var(DeclRefExpr * ref) {
  value = NULL;
  pos = new Position(ref->getLocation());
  scope = CurScope;
  decl_var = scope->find_var(ref->getDecl());
}

Access_Var::Access_Var(ArraySubscriptExpr * ArrEx) {
  value = NULL;
  Expr *base = ArrEx->getBase()->IgnoreImpCasts();
  index.push_back(ArrEx->getIdx()->IgnoreImpCasts());
  decl_var = NULL;

  while (isa<ArraySubscriptExpr>(base)) {
    ArraySubscriptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
    index.push_back(a->getIdx()->IgnoreImpCasts());
    base = a->getBase()->IgnoreImpCasts();
  }

  if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(base)) {
    pos = new Position(ref->getLocation());
    scope = CurScope;
    decl_var = scope->find_var(ref->getDecl());
  }
  else {
    ArrEx->dump();
    std::cerr <<"Exception Array\n";
  }
}

Access_Var::Access_Var(VarDecl * d) {
  value = NULL;
  pos = new Position(d->getLocation());
  scope = CurScope;
  decl_var = scope->find_var(d);
}

Access_Var::Access_Var() {
  decl_var = NULL;
  value = NULL;
}

Access_Var::~Access_Var() {
}

Access_Var * Access_Var::find_last_access(ValueDecl * decl_stmt, bool iswrite) {
  Decl_Var * decl_var = scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_last_access(*pos, iswrite);
}

Access_Var * Access_Var::find_next_access(ValueDecl * decl_stmt, bool iswrite) {
  Decl_Var * decl_var = scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_next_access(*pos, iswrite);
}


// Class ScopeIR implementation.
ScopeIR::ScopeIR() {
  info.loop = NULL;
  parent = NULL;
  condition = NULL;
  function = NULL;
}

ScopeIR::~ScopeIR() {

  for (auto& child : children)
    delete child;

//  for (auto& dv : decl_chain)
//    delete dv;

  for (auto& av : access_chain)
    delete av;

}

void ScopeIR::dump() {
  if (condition)
    condition->dump();

  if (function)
    function->decl_stmt->dump();
}


Decl_Var * ScopeIR::find_var(ValueDecl * decl_stmt){
  for (auto& dv : decl_chain) {
    if (dv->decl_stmt == decl_stmt)
      return dv;
  }

  if (parent == NULL)
    return NULL;

  return parent->find_var(decl_stmt);
}

void ScopeIR::add_child(ScopeIR * scope) {
  if (scope == NULL)
    return;
  scope->parent = this;
  children.push_back(scope);
}

void ScopeIR::addUDChain(Access_Var * acc_var) {
  if (acc_var == NULL)
    return;

  if (acc_var->isValid()){
    access_chain.push_back(acc_var);
  
    Decl_Var * var = acc_var->get_decl();
    var->append_access(acc_var);
  }
}
bool ScopeIR::setLoopInfo (LoopInfo * loop) {
  if (type == 2)  {
    info.loop = loop;
    loop->scope = this;
    return true;
  }
  else 
    return false;
}
bool ScopeIR::setFunctionInfo (FunctionInfo * func) {
  if (type == 1) {
    info.func = func;
    func->scope = this;
    return true;
  }
  else
    return false;
}
LoopInfo * ScopeIR::getLoopInfo() {
  if (type == 2) 
    return info.loop; 
  else 
    return NULL;
}
FunctionInfo * ScopeIR::getFunctionInfo() {
  if (type == 1) 
    return info.func; 
  else 
    return NULL;
}

//Compare two algebra expression has the same value in this context.
//TODO:We do not trace the compute chain right now.
bool ScopeIR::compareAlgebraExpr(Expr * e1, Expr * e2) {
  ExprSerializer se; 

  se.clear();
  se.TraverseStmt(e1);
  std::vector<Expr *> se_e1 = se.getOutput();

  se.clear();
  se.TraverseStmt(e2);
  std::vector<Expr *> se_e2 = se.getOutput();

  if (se_e1.size() != se_e2.size())
    return false;

  for (unsigned int i = 0; i < se_e1.size(); i++) {
    if (se_e1[i]->getStmtClass() != se_e2[i]->getStmtClass())
      return false;
    switch (se_e1[i]->getStmtClass()) {
      case Stmt::BinaryOperatorClass:
      case Stmt::CompoundAssignOperatorClass:
	{
	  BinaryOperator * BinOp1 = cast<BinaryOperator>(se_e1[i]);
	  BinaryOperator * BinOp2 = cast<BinaryOperator>(se_e2[i]);
	  if ( BinOp1->getOpcode() != BinOp2->getOpcode())
	    return false;
	}
	break;
      case Stmt::UnaryOperatorClass:
	{
          UnaryOperator *UnOp1 = dyn_cast<UnaryOperator>(se_e1[i]);
          UnaryOperator *UnOp2 = dyn_cast<UnaryOperator>(se_e2[i]);
	  if (UnOp1->getOpcode() != UnOp2->getOpcode())
	    return false;
	}
	break;
      case Stmt::DeclRefExprClass:
	{
	  DeclRefExpr * DeRef1 = cast<DeclRefExpr> (se_e1[i]);
	  DeclRefExpr * DeRef2 = cast<DeclRefExpr> (se_e2[i]);
	  if (DeRef1->getDecl() != DeRef2->getDecl()) 
	    return false;
	}
	break;
      case Stmt::IntegerLiteralClass:
	{
          IntegerLiteral * int1 = cast<IntegerLiteral>(se_e1[i]);
          llvm::APInt llvm_val1 = int1->getValue();

          IntegerLiteral * int2 = cast<IntegerLiteral>(se_e2[i]);
          llvm::APInt llvm_val2 = int2->getValue();
 
	  if (llvm_val1.eq(llvm_val2) == false)
	    return false;
	}
	break;
      case Stmt::FloatingLiteralClass:
	{
          FloatingLiteral * float1 = cast<FloatingLiteral>(se_e1[i]);
          llvm::APFloat llvm_val1 = float1->getValue();

          FloatingLiteral * float2 = cast<FloatingLiteral>(se_e2[i]);
          llvm::APFloat llvm_val2 = float2->getValue();

 	  if (llvm_val1.compare(llvm_val2) != llvm::APFloatBase::cmpEqual)
	    return false;
	}
	break;
      case Stmt::ImaginaryLiteralClass:
      case Stmt::ParenExprClass:
      case Stmt::ArraySubscriptExprClass:
      case Stmt::MemberExprClass:
	break;
      default:
	break;
    }
  }

  return true;
}

//Find if the Algebra expression is related to Var, it will trace the compute chain.
bool ScopeIR::isVarRelatedExpr(Expr * e, ValueDecl * v) {
  ExprSerializer se; 
  se.clear();
  se.TraverseStmt(e);
  std::vector<Expr *> se_e = se.getOutput();

  std::vector<ValueDecl *> se_v;
  for (unsigned int i = 0; i < se_e.size(); i++) {
    if(DeclRefExpr * dr = dyn_cast<DeclRefExpr> (se_e[i])) {
      ValueDecl * d = dr->getDecl();
      if (d == v)
	return true;
      else if (d->getKind() == clang::Decl::Var) {
	Decl_Var *dv = find_var(d);
	Position pos(e->getLocStart());
	Access_Var *av = dv->find_last_access(pos, true);
	if (av && isVarRelatedExpr(av->get_value(), v))
	  return true;
      }
    }
  }

  return false;
}

unsigned int ScopeIR::get_call_num() {
  unsigned int sum = callers.size();
  for (auto& child : children) 
    sum += child->get_call_num();

  return sum;
}

bool ScopeIR::isInside(ScopeIR *s) {
  if (this == s)
    return true;

  for (auto&child : children)
    if (child->isInside(s) == true)
      return true;
  
  return false;
}
