//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#ifndef KERNELEXTRACTOR_H
#define KERNELEXTRACTOR_H

#include "ClangHeader.h"
#include "KernelInfo.h"

extern File_Info f_info;

class KernelExtractor : public RecursiveASTVisitor<KernelExtractor> {
private:
  //To find is var is used in expr e.
  bool find_usage (Expr * e, ValueDecl * var);
  //Handle DelcRef in index expr.
  int analysis_declref (DeclRefExpr *ref, std::string &min, std::string &max);
  //Handle ParenExpr in BinaryOperator.
  int analysis_paren (Expr *idx, std::string &min, std::string &max);
  //Analysis binary operator, give its min value and max value.
  int analysis_bin_op (BinaryOperator *op, std::string &min, std::string &max);
  //Analysis Expr vector(arry's 1st dim index), return its min value and max value.
  //Because the type of mult-array is a pointer that point to 
  //(dim -1) mult-array
  int analysis_index (Expr * idx, std::string &min, std::string &max);
  void evalue_index (Expr *idx, struct var_data *cur_var);
  //Analysis ArraySubscriptExpr, maybe mult dim array.
  void analysis_array (ArraySubscriptExpr *arr, std::string &base_name,
			std::vector<Expr *> &idx_arry);
  //Check memory read/write, return var name.
  //write name only occurs when op is "=", and lvalue is the write memory.
  //All other mem access is read.
  //return 0: is a assignment opt. 1: others.
  //return 2: *=/+= compound assignment.
  int check_mem_access (BinaryOperator *op, std::string &lhs,
			std::vector<Expr *> &lhs_idx,
			std::string &rhs, 
			std::vector<Expr *> &rhs_idx);
  //Get the spelling string for Stmt *s.
  std::string get_str(Stmt *s);
  //return which level scope define this var, 0 means in kernel, 1 means out of kernel.
  unsigned query_var (std::string name, struct var_data ** var);
  struct var_data * Insert_var_data (VarDecl *d, 
				struct Scope_data &cur_scope);
  void clean_kernel_info ();

public:
  KernelExtractor(std::vector<struct Kernel_Info> &k, std::vector<struct Scope_data> &s_stack): k_info_queue(k), Scope_stack(s_stack)  {
    clean_kernel_info ();
  }
  void Initialize(ASTContext &Context) {
    Ctx = &Context;
    SM = &(Ctx->getSourceManager());
  }
  //Invoked before visiting a statement or expression.
  //Return false to skip visiting the node.
  bool dataTraverseStmtPre (Stmt *st);
  //Invoked after visiting a statement or expression via data recursion.
  //return false if the visitation was terminated early.
  bool dataTraverseStmtPost (Stmt *st);
  bool VisitStmt(Stmt *s);
  bool VisitVarDecl(VarDecl *d);
  bool VisitFunctionDecl (FunctionDecl *f);

private:
  struct Kernel_Info k_info;
  std::vector<struct Kernel_Info> &k_info_queue;
  std::vector<struct Scope_data> &Scope_stack;
};

#endif
