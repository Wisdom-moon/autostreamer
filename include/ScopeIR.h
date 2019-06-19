//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

//===----------------------------------------------------------------------===//
//
// ScopeIR: generate ScopeIR from AST. 
//
//===----------------------------------------------------------------------===//
#ifndef SCOPEIR_H
#define SCOPEIR_H

#include "ClangHeader.h"

class ScopeIR;
class Access_Var;
class Decl_Var;
class LoopInfo;
class FunctionInfo;
class ExprSerializer;

class Position {
  private:
  unsigned int col;
  unsigned int line;

  public:

  Position();
  Position(SourceLocation loc);
  ~Position();
  bool operator<(const Position & pos);
  bool operator>(const Position & pos);
  unsigned int get_line() {return line;}
};

class ValueRange {
  private:
  //From conditional expression in if or for,  we can get the lower or upper boundary.
  Expr * lower_boundary;
  int lower_offset;
  Expr * upper_boundary;
  int upper_offset;

  public:
  ValueRange();
  ValueRange(Expr * e);
  ~ValueRange();
};

class Decl_Var{
  private:

  Position *pos;
  // 0 means function decl; 
  // 1 means scalar function parameter; 
  // 2 means pointer function parameter; 
  // 3 means scalar variable;
  // 4 means pointer variable.
  int category; 
  std::string name;
  std::string type; // Type of variable.
  std::vector <Access_Var *> access_chain;

  public:
  ValueDecl * decl_stmt;
  ScopeIR * scope;
  ValueRange * init_value;

  Decl_Var();
  Decl_Var(ValueDecl * decl);
  ~Decl_Var();
  Access_Var * find_last_access(Position &pos, bool isWrite);
  Access_Var * find_next_access(Position &pos, bool isWrite);
  void append_access(Access_Var * av) {access_chain.push_back(av);}
  Position * get_pos() {return pos;}
};

class Access_Var {
  private:

  Decl_Var * decl_var;
  ValueRange *value_range; // For write;
  Expr * value;

  public:
  ScopeIR * scope;
  Position *pos;
  std::vector <Expr *> index;//For array or memory pointer

  Access_Var(ArraySubscriptExpr * ArrEx);
  Access_Var(DeclRefExpr * ref);
  Access_Var(VarDecl * d);
  Access_Var();
  ~Access_Var();
  Access_Var * find_last_access(ValueDecl * decl_stmt, bool isWrite);
  Access_Var * find_next_access(ValueDecl * decl_stmt, bool isWrite);
  Decl_Var * get_decl(){return decl_var;}
  bool isValid() {return (decl_var != NULL);}
  void set_index(Expr * e) {index.clear(); index.push_back(e);}
  void set_value (Expr *e) {value = e;}
  Expr * get_value () {return value;}
  
  bool isWrite() {return value ? true: false;}
  bool isMem() {return index.empty() ? false : true;}
};

class ScopeIR {
  private:

  std::vector <Decl_Var *> decl_chain;

  union {
    LoopInfo * loop;
    FunctionInfo * func;
  } info;

  public:
  //Start and End position of this scope;
  Position *s_pos;
  Position *e_pos;

  //-1: Program scope;
  //0: One file scope; 1: function; 2: for loop; 3: do loop; 4: while loop;
  //5: if stmt; 6: switch stmt; 7: switch case;
  //8: Try; 9: Catch; 10: SEH; 11: compound; 12: others.
  //13: omp parallel for; 14: captured stmt;
  //15: if then part; 16: if else part;
  unsigned int type;
  ScopeIR * parent;
  Stmt * condition; //If, For, switch and  case condition statement which lead to a condition execution.
  Decl_Var * function;
  int last_return_line; //Just for function.
  std::vector<CallExpr *> callers;

  std::vector<ScopeIR *> children;
  std::vector <Access_Var *> access_chain;

  ScopeIR();
  ~ScopeIR();
  Decl_Var * find_var(ValueDecl * decl_stmt);
  Decl_Var * get_var(unsigned int n) {return decl_chain[n];}
  void add_child(ScopeIR * scope);

  void addUDChain(Access_Var * acc_var);
  void append_decl(Decl_Var * dv) {decl_chain.push_back(dv); dv->scope = this;}

  bool setLoopInfo (LoopInfo * loop); 
  bool setFunctionInfo (FunctionInfo * func);
  LoopInfo * getLoopInfo();
  FunctionInfo * getFunctionInfo();

  //Compare two algebra expression has the same value in this context.
  bool compareAlgebraExpr(Expr * e1, Expr * e2);
  //Find if the Algebra expression is related to Var, it will trace the compute chain.
  bool isVarRelatedExpr(Expr * e, ValueDecl * v);
  Position * get_start_pos () {return s_pos;}

  void dump(); 
  unsigned int get_call_num();

  bool isInside(ScopeIR *s);
};

extern ScopeIR * TopScope;
extern ScopeIR * CurScope;

#endif
