//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "Scope.h"

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
  return (line > pos.line || (line == pos.line && col k pos.col));
}

// Class Value_Range Implementation.
Value_Range::Value_Range() {
}

Value_Range::~Value_Range() {
}

Value_Range::Value_Range(Expr *e) {
  lower_boundary = e;
  lower_offset = 0;
  upper_boundary = e;
  upper_offset = 0;
}

bool Value_Range::isRelated(Decl_Var * d){

  return true;
}

// Class Decl_Var Implementation.
// For each variable declar.
Decl_Var::Decl_Var() {
}

Decl_Var::Decl_Var(Decl * d) {
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
	if (d->hasInit()) {
	  init_value = new ValueRange(d->getInit()->IgnoreImpCasts());
	  CurScope->add_UDChain(d->getInit()->IgnoreImpCasts(), false);
	}
	break;
    default:
	break;
  }

  if (category > 0 && type.find("*") != std::string::npos)
    category ++;

}

Decl_Var::~Decl_Var() {
  for (int i = 0; i < init_value.size(); i++)
    delete init_value[i];
}

//TODO: We have not consider the loop context;
Access_Var * Decl_Var::find_last_access(Position &pos, bool iswrite){
  for (int i = access_chain.size() - 1; i >= 0; i++) {
    if (access_chain[i]->pos < pos && access_chain[i]->isWrite() == iswrite)
      return access_chain[i];
  }

  return NULL:
}

Access_Var * Decl_Var::find_next_access(Position &pos, bool iswrite){
  for (int i = 0; i < access_chain.size(); i++) {
    if (access_chain[i]->pos > pos && access_chain[i]->isWrite() == iswrite)
      return access_chain[i];
  }

  return NULL:
}


// Class Access_Var Implementation.
// For each access of one variable.
Access_Var::Access_Var(DeclRefExpr * ref) {
  pos = new Position(ref->getLocation());
  ptr_scope = CurScope;
  ptr_decl = ref->getDecl();
}

Access_Var::Access_Var(ArraySubscriptExpr * ArrEx) {
  Expr *base = ArrEx->getBase()->IgnoreImpCasts();
  index.push_back(ArrEx->getIdx()->IgnoreImpCasts());
  ptr_decl = NULL;

  while (isa<ArraySubscriptExpr>(base)) {
    ArraySubsciptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
    index.push_back(a->getIdx()->IgnoreImpCasts());
    base = a->getBase()->IgnoreImpCasts();
  }

  if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(base)) {
    pos = new Position(ref->getLocation());
    ptr_scope = CurScope;
    ptr_decl = ref->getDecl();
  }
  else {
    ArrEx->dump();
    cerr <<"Exception Array\n";
  }
}

Access_Var::Access_Var() {
  ptr_decl = NULL;
}

Access_Var::~Access_Var() {
}
void Access_Var::set_var(DeclRefExpr *d) {
  pos = new Position(ref->getLocation());
  ptr_scope = CurScope;
  ptr_decl = ref->getDecl();
}

Access_Var * Access_Var::find_last_access(Decl * decl_stmt, bool iswrite) {
  Decl_Var * decl_var = ptr_scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_last_access(pos, iswrite);
}

Access_Var * Access_Var::find_next_access(Decl * decl_stmt, int iswrite) {
  Decl_Var * decl_var = ptr_scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_next_access(pos, iswrite);
}


// Class ScopeIR implementation.
ScopeIR::ScopeIR() {
  info.loop = NULL;
  parent = NULL;
}

ScopeIR::~ScopeIR() {

  for (int i = 0; i < children.size(); i++)
    delete children[i];

  for (int i = 0; i < decl_chain.size(); i++)
    delete decl_chain[i];

  for (int i = 0; i < access_chain.size(); i++)
    delete access_chain[i];

}


Decl_Var * ScopeIR::find_var(Decl * decl_stmt){
  for (int i = 0; i < decl_chain.size(); i++) {
    if (decl_chain[i]->decl_stmt == decl_stmt)
      return decl_chain[i];
  }

  if (ptr_parent == NULL)
    return NULL;

  return ptr_parent->find_var(decl_stmt);
}

void ScopeIR::add_child(ScopeIR * scope) {
  if (scope == NULL)
    return;
  scope->parent = this;
  children.push_back(scope);
}

void ScopeIR::add_UDChain(Access_Var * acc_var) {
  if (acc_var == NULL)
    return;

  if (acc_var->isValid()){
    access_chain.push_back(acc_var);
  
    Decl_Var * var = CurScope->find_var(acc_var->get_decl);
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

  for (int i = 0; i < se_e1.size(); i++) {
    if (se_e1[i]->getStmtClass() != se_e2[i]->getStmtClass())
      return false;
    switch (se_e1[i]->getStmtClass()) {
      case clang::Stmt::BinaryOperator:
      case clang::Stmt::CompoundAssignOperator:
	BinaryOperator * BinOp1 = cast<BinaryOperator>(se_e1[i]);
	BinaryOperator * BinOp2 = cast<BinaryOperator>(se_e2[i]);
	if ( BinOp1->getOpcode() != BinOp2->getOpcode())
	  return false;
      case clang::Stmt::UnaryOperator:
        UnaryOperator *UnOp1 = dyn_cast<UnaryOperator>(se_e1[i])
        UnaryOperator *UnOp2 = dyn_cast<UnaryOperator>(se_e2[i])
	if (UnOp1->getOpcode() != UnOp2->getOpcode())
	  return false;
      case clang::Stmt::DeclRefExpr:
	DeclRefExpr * DeRef1 = cast<DeclRefExpr> (se_e1[i]);
	DeclRefExpr * DeRef2 = cast<DeclRefExpr> (se_e2[i]);
	if (DeRef1->getDecl() != DeRef2->getDecl()) 
	  return false;
      case clang::Stmt::IntegerLiteral:
        IntegerLiteral * int1 = cast<IntegerLiteral>(se_e1[i]);
        llvm::APInt llvm_val1 = int1->getValue();

        IntegerLiteral * int2 = cast<IntegerLiteral>(se_e2[i]);
        llvm::APInt llvm_val2 = int2->getValue();
 
	if (llvm_val1.eq(llvm_val2) == false)
	  return false;
      case clang::Stmt::FloatingLiteral:
        FloatingLiteral * float1 = cast<FloatingLiteral>(se_e1[i]);
        llvm::APFloat llvm_val1 = float1->getValue();

        FloatingLiteral * float2 = cast<FloatingLiteral>(se_e2[i]);
        llvm::APFloat llvm_val2 = float2->getValue();

 	if (llvm_val1.compare(llvm_val2) != llvm::APFloatBase::cmpEqual)
	  return false;

      case clang::Stmt::ImaginaryLiteral:
      case clang::Stmt::ParenExpr:
      case clang::Stmt::ArraySubscriptExpr:
      case clang::Stmt::MemberExpr:
	break;
    }
  }

  return true;
}

//Find if the Algebra expression is related to Var, it will trace the compute chain.
bool ScopeIR::isVarRelatedExpr(Expr * e, Decl * v) {
  ExprSerializer se; 
  se.clear();
  se.TraverseStmt(e1);
  std::vector<Expr *> se_e = se.getOutput();

  std::vector<Decl *> se_v;
  for (int i = 0; i < se_e.size(); i++) {
    if(DeclRefExpr * DecRef = dyn_cast<DeclRefExpr> (se_e[i])) {
      Decl * d = DeclRef->getDecl();
      if (d == v)
	return true;
      else {
	Decl_Var *dv = find_var(d);
	Access_Var *av = dv.find_last_access(Position pos(e->getLocation), true);
	if (av && isVarRelatedExpr(av->get_value(), v))
	  return true;
      }
    }
  }

  return false;
}
