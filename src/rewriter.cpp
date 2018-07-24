//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "rewriter.h"

//#define DEBUG_INFO

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.


class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
private:

  //To find is var is used in expr e.
  bool find_usage (Expr * e, ValueDecl * var) {
    e = e->IgnoreImpCasts();
    if (isa<DeclRefExpr>(e)) {
      DeclRefExpr *ref = cast<DeclRefExpr> (e);
      if (ref->getDecl() == var)
	return true;
    }
    else if (isa<BinaryOperator>(e)) {
      BinaryOperator *op = cast<BinaryOperator>(e);
      if (find_usage (op->getLHS(), var))
        return true;
      if (find_usage (op->getRHS(), var))
        return true;
    }
    else if (isa<UnaryOperator>(e)) {
      UnaryOperator *op = cast<UnaryOperator>(e);
      if (find_usage (op->getSubExpr(), var))
        return true;
    }

    return false;
  }

  
  //Handle DelcRef in index expr.
  int analysis_declref (DeclRefExpr *ref, std::string &min, std::string &max) {
    int ret = 3;
    struct var_data *cur_var;
    query_var (ref->getDecl()->getName().str(), &cur_var);
    if (cur_var && cur_var->type == 1) {
      if (!cur_var->min_value_str.empty())
	  min += cur_var->min_value_str;
      else
          min += cur_var->name;
      if (!cur_var->max_value_str.empty())
          max += cur_var->max_value_str;
      else
          max += cur_var->name;
    }

    return ret;
  }
  //Handle ParenExpr in BinaryOperator.
  int analysis_paren (Expr *idx, std::string &min, std::string &max) {
    int ret = 0;
    if (isa<ParenExpr> (idx)) {
      Expr *e = cast<ParenExpr> (idx)->getSubExpr()->IgnoreImpCasts();
      min += "(";
      max += "(";
      if (isa<BinaryOperator> (e)) {
	ret = analysis_bin_op (cast<BinaryOperator> (e), min, max);
      }
      else if (isa <ParenExpr> (e)) {
	ret = analysis_paren (e, min, max);
      }
      else if (isa <DeclRefExpr> (e)) {
        DeclRefExpr *ref = cast<DeclRefExpr>(e);
	ret = analysis_declref (ref, min, max);
      }
      else {
        min += get_str(e);
        max += get_str(e);
      }
      min += ")";
      max += ")";
    }
    return ret;
  }
  //Analysis binary operator, give its min value and max value.
  int analysis_bin_op (BinaryOperator *op, std::string &min, std::string &max) {

    int ret = 0;
    int l_ret = 0;
    int r_ret = 0;

    Expr *lhs = op->getLHS()->IgnoreImpCasts();
    if (isa<BinaryOperator> (lhs))
      l_ret = analysis_bin_op (cast<BinaryOperator>(lhs), min, max);
    else if (isa<DeclRefExpr> (lhs)){
      DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
      l_ret = analysis_declref (ref, min, max);
    }
    else if (isa<ParenExpr> (lhs)) {
      l_ret = analysis_paren(lhs, min, max);
    }
    else if (isa<IntegerLiteral> (lhs)) {
      IntegerLiteral * int_lit = cast<IntegerLiteral>(lhs);
      llvm::APInt int_val = int_lit->getValue();
      int * p_val = (int *)int_val.getRawData();
      l_ret = *p_val;
      min += std::to_string (l_ret);
      max += std::to_string (l_ret);
    }
    else {
      min += get_str(lhs);
      max += get_str(lhs);
    }

    std::string op_str = op->getOpcodeStr().str();
    min += op_str;
    max += op_str;

    Expr *rhs = op->getRHS()->IgnoreImpCasts();
    if (isa<BinaryOperator> (rhs))
      r_ret = analysis_bin_op (cast<BinaryOperator>(rhs), min, max);
    else if (isa<DeclRefExpr> (rhs)){
      DeclRefExpr *ref = cast<DeclRefExpr>(rhs);
      r_ret = analysis_declref (ref, min, max);
    }
    else if (isa<ParenExpr> (rhs)) {
      r_ret = analysis_paren(rhs, min, max);
    }
    else if (isa<IntegerLiteral> (rhs)) {
      IntegerLiteral * int_lit = cast<IntegerLiteral>(rhs);
      llvm::APInt int_val = int_lit->getValue();
      int * p_val = (int *)int_val.getRawData();
      r_ret = *p_val;
      min += std::to_string (r_ret);
      max += std::to_string (r_ret);
    }
    else {
      min += get_str(rhs);
      max += get_str(rhs);
    }

    switch (op->getOpcode ()) {
      case BO_Mul:
	ret = l_ret * r_ret;
	break;
      case BO_Add:
	ret = l_ret + r_ret;
	break;
      case BO_Sub:
	ret = l_ret - r_ret;
	break;
      default:
	break;
    }

    return ret;
  }

  //Analysis Expr vector(arry's 1st dim index), return its min value and max value.
  //Because the type of mult-array is a pointer that point to 
  //(dim -1) mult-array
  int analysis_index (Expr * idx, std::string &min, std::string &max) {
    int ret = 0;
    if (idx == NULL)
      return ret;

    min.clear();
    max.clear();

    if (isa<BinaryOperator>(idx)) {
      ret = analysis_bin_op (cast<BinaryOperator>(idx), min, max);
    }
    else if (isa<DeclRefExpr> (idx)){
      DeclRefExpr *ref = cast<DeclRefExpr>(idx);
      ret = analysis_declref (ref, min, max);
      }
    else if (isa<ParenExpr> (idx)) {
      ParenExpr * paren = cast<ParenExpr> (idx);
      ret = analysis_paren (paren, min, max);
    }
    else {
      min = get_str(idx);
      max = get_str(idx);
    }

    return ret;
  }
  void evalue_index (Expr *idx, struct var_data *cur_var) {
    std::string min;
    std::string max;
    int value = analysis_index (idx, min, max);

    if (cur_var->IdxChains.size() == 1) {
      cur_var->value_min = value;
      cur_var->value_max = value;
      cur_var->max_value_str = max;
      cur_var->min_value_str = min;
    }
    else if (value > cur_var->value_max) {
      cur_var->value_max = value;
      cur_var->max_value_str = max;
    }
    else if (value < cur_var->value_min) {
      cur_var->value_min = value;
      cur_var->min_value_str = min;
    }
  }

  //Analysis ArraySubscriptExpr, maybe mult dim array.
  void analysis_array (ArraySubscriptExpr *arr, std::string &base_name,
			std::vector<Expr *> &idx_arry) {
    Expr *base = arr->getBase()->IgnoreImpCasts();
    Expr *idx  = arr->getIdx()->IgnoreImpCasts();
    if (isa<DeclRefExpr>(base)) {
      DeclRefExpr *ref = cast<DeclRefExpr>(base);
      base_name = ref->getDecl()->getName().str();
      idx_arry.push_back(idx);
    }
    else if (isa<ArraySubscriptExpr>(base)){
      analysis_array (cast<ArraySubscriptExpr>(base), base_name, idx_arry);
      idx_arry.push_back(idx);
    }
    return ;
  }

  //Check memory read/write, return var name.
  //write name only occurs when op is "=", and lvalue is the write memory.
  //All other mem access is read.
  //return 0: is a assignment opt. 1: others.
  //return 2: *=/+= compound assignment.
  int check_mem_access (BinaryOperator *op, std::string &lhs,
			std::vector<Expr *> &lhs_idx,
			std::string &rhs, 
			std::vector<Expr *> &rhs_idx) {
    int ret = 1;
    if (op->isAssignmentOp()) {
      ret = 0;
    }
    if (op->isCompoundAssignmentOp()) {
      ret = 2;
    }

    if (isa<ArraySubscriptExpr>(op->getRHS()->IgnoreImpCasts())) {
      ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(op->getRHS()->IgnoreImpCasts());
      analysis_array (arr, rhs, rhs_idx);
    }
    else if (isa<DeclRefExpr>(op->getRHS()->IgnoreImpCasts())) {
      DeclRefExpr *ref = cast<DeclRefExpr>(op->getRHS()->IgnoreImpCasts());
      struct var_data *cur_var;
      query_var (ref->getDecl()->getName().str(), &cur_var);
      if (cur_var && cur_var->type == 0)
        rhs = ref->getDecl()->getName().str();
    }
    else if (isa<MemberExpr>(op->getRHS()->IgnoreImpCasts())) {
      MemberExpr * member = cast<MemberExpr>(op->getRHS()->IgnoreImpCasts());
      if (isa<ArraySubscriptExpr>(member->getBase())) {
        ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(member->getBase());
        analysis_array (arr, rhs, rhs_idx);
      }
    }

    if (isa<ArraySubscriptExpr>(op->getLHS()->IgnoreImpCasts())) {
      ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(op->getLHS()->IgnoreImpCasts());
      analysis_array (arr, lhs, lhs_idx);
    }
    else if (isa<DeclRefExpr>(op->getLHS()->IgnoreImpCasts())) {
      DeclRefExpr *ref = cast<DeclRefExpr>(op->getLHS()->IgnoreImpCasts());
      struct var_data *cur_var;
      query_var (ref->getDecl()->getName().str(), &cur_var);
      if (cur_var && cur_var->type == 0)
        lhs = ref->getDecl()->getName().str();
    }
    else if (isa<MemberExpr>(op->getLHS()->IgnoreImpCasts())) {
      MemberExpr * member = cast<MemberExpr>(op->getLHS()->IgnoreImpCasts());
      if (isa<ArraySubscriptExpr>(member->getBase())) {
        ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(member->getBase());
        analysis_array (arr, lhs, lhs_idx);
      }
    }

    return ret;
  }

  //Get the spelling string for Stmt *s.
  std::string get_str(Stmt *s){
    std::string ret;

    if (isa<DeclRefExpr>(s)) {
      DeclRefExpr *ref = cast<DeclRefExpr>(s);
      ret = ref->getDecl()->getName().str();
    }
    else {
      //Convert into ExpansionLoc, is because s may be Macro and be expanded.
      //the default is SpellingLoc, which is the Macro define line.
      //ExpansionLoc is the loc that be expanded.
      std::string str1 = SM->getCharacterData(SM->getExpansionLoc(s->getLocStart()));
      std::string str2 = SM->getCharacterData(SM->getExpansionLoc(s->getLocEnd()));
      unsigned len = str1.size() - str2.size() + 1;
      ret = str1.substr(0, len);
    }
    return ret;
  }
  //return which level scope define this var, 0 means in kernel, 1 means out of kernel.
  unsigned query_var (std::string name, struct var_data ** var) {
    unsigned ret = 0;
    unsigned level = 0;
    unsigned size = Scope_stack.size();
    *var = NULL;
    for (level = 0; level < size; level ++) {
      if (Scope_stack[size - level - 1].type == 13)
	ret = 1;
      if (Scope_stack[size - level - 1].var_table.find(name) != Scope_stack[size - level - 1].var_table.end())
	break;
    }

    if (level < size) {
      *var = &((*(Scope_stack[size - level - 1].var_table.find(name))).second);
    }

    if (process_state != 1)
      ret = 1;

    return ret;
  }

  struct var_data * Insert_var_data (VarDecl *d, 
				struct Scope_data &cur_scope) {
    struct var_data * new_var;
    if (cur_scope.var_table.find(d->getName().str()) == cur_scope.var_table.end()) {
      struct var_data new_var;
      new_var.decl_stmt = d;
      new_var.name = d->getName().str();
      new_var.type_str = d->getType().getAsString();
      int f_index;
      if ((f_index = new_var.type_str.find("*")) >= 0)
        new_var.type = 0;
      else {
        new_var.type = 1;
        new_var.size_str = "sizeof(";
        new_var.size_str += new_var.type_str;
        new_var.size_str += ")";
      }

      new_var.usedByKernel = 0;
      new_var.isKernelArg = false;
      cur_scope.var_table.insert({new_var.name, new_var});
    }

    new_var = &((*(cur_scope.var_table.find(d->getName().str()))).second);

    return new_var;
  }

  void clean_kernel_info () {
    k_info.enter_loop = 0;
    k_info.exit_loop = 0;
    k_info.init_cite = 0;
    k_info.finish_cite = 0;
    k_info.create_mem_cite = 0;
    k_info.replace_line = 0;
    k_info.mem_bufs.clear();
    k_info.val_parms.clear();
    k_info.pointer_parms.clear();
    k_info.length_var.clear();
    k_info.local_parms.clear();
    k_info.start_index.clear();
    k_info.loop_index = NULL;
    k_info.insns = 0;
  }


public:
  MyASTVisitor(std::vector<struct Kernel_Info> &k, std::vector<struct Scope_data> &s_stack): k_info_queue(k), Scope_stack(s_stack)  {
    process_state = 0;
    clean_kernel_info ();
  }
  void Initialize(ASTContext &Context) {
    Ctx = &Context;
    SM = &(Ctx->getSourceManager());
  }

  //Invoked before visiting a statement or expression.
  //Return false to skip visiting the node.
  bool dataTraverseStmtPre (Stmt *st) {

      struct Scope_data &cur_scope = Scope_stack.back();
      struct Scope_data new_scope;
      new_scope.p_func = NULL;
      new_scope.sline = SM->getExpansionLineNumber(st->getLocStart());
      new_scope.scol = SM->getExpansionColumnNumber(st->getLocStart());
      new_scope.eline = SM->getExpansionLineNumber(st->getLocEnd());
      new_scope.ecol = SM->getExpansionColumnNumber(st->getLocEnd());

    if (isa<CompoundStmt>(st) || isa<WhileStmt>(st) || isa<CXXCatchStmt>(st)
       || isa<CXXForRangeStmt>(st) || isa<CXXTryStmt>(st) || isa<DoStmt>(st)
       || isa<ForStmt>(st) || isa<IfStmt>(st) || isa<SEHExceptStmt>(st)
       || isa<SEHFinallyStmt>(st) || isa<SEHFinallyStmt>(st)
       || isa<SwitchCase>(st) || isa<SwitchStmt>(st) || isa<WhileStmt>(st)
       || isa<CapturedStmt>(st)) {
      if (isa<CompoundStmt>(st)) {
	CompoundStmt *c_st = cast<CompoundStmt>(st);
	if (cur_scope.p_func) {
	  new_scope.type = 1;
          k_info.init_cite = SM->getExpansionLineNumber(
					c_st->body_front()->getLocStart());
          k_info.finish_cite = SM->getExpansionLineNumber(
                                        c_st->getLocEnd());
	  //Add ParmVarDecl to new_scope.var_table
	  if (SM->isWrittenInMainFile (cur_scope.p_func->getLocation())) {
	    for (unsigned i = 0; i < cur_scope.p_func->getNumParams(); i++) {
	      Insert_var_data(cur_scope.p_func->getParamDecl(i), new_scope);
	    }
 	  }
	}
	else
	  new_scope.type = 11;
      }
      else if (isa<ForStmt>(st))
	new_scope.type = 2;
      else if (isa<DoStmt>(st))
	new_scope.type = 3;
      else if (isa<WhileStmt>(st))
	new_scope.type = 4;
      else if (isa<IfStmt>(st))
	new_scope.type = 5;
      else if (isa<SwitchStmt>(st))
	new_scope.type = 6;
      else if (isa<SwitchCase>(st))
	new_scope.type = 7;
      else if (isa<CXXTryStmt>(st))
	new_scope.type = 8;
      else if (isa<CXXCatchStmt>(st))
	new_scope.type = 9;
      else if (isa<SEHFinallyStmt>(st) || isa<SEHFinallyStmt>(st)
		|| isa<SEHExceptStmt>(st))
	new_scope.type = 10;
      else if (isa<CXXForRangeStmt>(st))
	new_scope.type = 12;
      else if (isa<CapturedStmt>(st)) {
	new_scope.type = 14;
        if (cur_scope.type == 13)
	  process_state = 1;
      }

      Scope_stack.push_back(new_scope);
    }
    Stmt::StmtClass sc = st->getStmtClass();
    if (sc == clang::Stmt::OMPParallelForDirectiveClass) {
      k_info.enter_loop = SM->getExpansionLineNumber(st->getLocStart());
      new_scope.type = 13;
      Scope_stack.push_back(new_scope);
    }

    return true;
  }
 
  //Invoked after visiting a statement or expression via data recursion.
  //return false if the visitation was terminated early.
  bool dataTraverseStmtPost (Stmt *st) {
    if (isa<CompoundStmt>(st) || isa<WhileStmt>(st) || isa<CXXCatchStmt>(st)
       || isa<CXXForRangeStmt>(st) || isa<CXXTryStmt>(st) || isa<DoStmt>(st)
       || isa<ForStmt>(st) || isa<IfStmt>(st) || isa<SEHExceptStmt>(st)
       || isa<SEHFinallyStmt>(st) || isa<SEHFinallyStmt>(st)
       || isa<SwitchCase>(st) || isa<SwitchStmt>(st) || isa<WhileStmt>(st)
       || isa<CapturedStmt>(st)) {

      Scope_stack.pop_back();
      if (isa<CapturedStmt>(st)) {
	if (Scope_stack.back().type == 13) {
          //Add mem_xfer to kernel_info here.
	  struct mem_xfer new_mem;
	  unsigned last_decl_line = 0;
	  for (auto &scope : Scope_stack) {
	    for (auto &val_pair : scope.var_table) {
	      struct var_data cur_var = val_pair.second;
	      new_mem.buf_name = cur_var.name;
	      new_mem.type_name = cur_var.type_str;
	      int idx = new_mem.type_name.find("const");
              if (idx >= 0)
		new_mem.type_name.erase(idx, 5);
	      new_mem.elem_type = new_mem.type_name;
	      idx = new_mem.elem_type.find("(*)");
	      if (idx >= 0) {
		new_mem.elem_type.erase(idx, 3);
	      }
	      else if ((idx =  new_mem.elem_type.find("*")) >= 0)
		new_mem.elem_type.erase(idx, 1);

	      if (!cur_var.size_str.empty())
	        new_mem.size_string = cur_var.size_str;
	      //mem buf size only be decide by max_value.
	      else if (!cur_var.max_value_str.empty())
	      {
	    	new_mem.size_string = "(";
		new_mem.size_string += cur_var.max_value_str;
		new_mem.size_string += "+ 1)";
		new_mem.size_string += "* sizeof (";
		new_mem.size_string += new_mem.elem_type;
		new_mem.size_string += ")";
	      }
	      //is_overlap == true means have data overlap between streams.
	      bool is_overlap = false;
	      //Need to fixup: how to analysis overlap memory access?
	      for (auto &idx_arry : cur_var.IdxChains) {
		Expr * idx = idx_arry[0];
 		if (idx && isa<DeclRefExpr>(idx)) {
		  DeclRefExpr *ref = cast<DeclRefExpr>(idx);
		  if (ref->getDecl() != k_info.loop_index) {
		    is_overlap = true;
		  }
		}
		else if (idx && find_usage (idx, k_info.loop_index))
 		  is_overlap = true;

		new_mem.dim = idx_arry.size();
	      }
	      if (cur_var.usedByKernel > 0) {
		unsigned decl_line = SM->getExpansionLineNumber(
					cur_var.decl_stmt->getLocStart());
		if (last_decl_line < decl_line)
		  last_decl_line = decl_line;
	      }
	      switch (cur_var.usedByKernel) {
		case 1:
	  	  new_mem.type = is_overlap ? 1 : 2;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		case 2:
	  	  new_mem.type = is_overlap ? 8 : 4;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		case 3:
	  	  new_mem.type = is_overlap ? 9 : 6;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		default:
		  break;
	      }
	    cur_var.usedByKernel = 0;
	    cur_var.isKernelArg = false;
	    cur_var.IdxChains.clear();
	    }
	  }
	  k_info.create_mem_cite = last_decl_line > k_info.init_cite ? last_decl_line : k_info.init_cite;
          k_info_queue.push_back(k_info);

          clean_kernel_info ();
    	  process_state = 2;
	}
      }
    }
    Stmt::StmtClass sc = st->getStmtClass();
    if (sc == clang::Stmt::OMPParallelForDirectiveClass) {
      Scope_stack.pop_back();
    }
    return true;
  }

  bool VisitStmt(Stmt *s) {

    //Location the outside for stmt in kernel, abstract iteration times
    if (isa<ForStmt>(s) && process_state == 1) {
      //Dump the target for loop, for debug.
      #ifdef DEBUG_INFO
      s->dump();
      #endif
      //Record iterator var's value range
      ForStmt *for_stmt = cast<ForStmt>(s);
      Stmt *init = for_stmt->getInit();
      if (isa<DeclStmt>(init)) {
	  VarDecl *var = cast<VarDecl>(cast<DeclStmt>(init)->getSingleDecl());
	  Expr * init_val = var->getInit();
          struct var_data * init_var = Insert_var_data(var, Scope_stack.back());
	  analysis_index (init_val, init_var->min_value_str, init_var->max_value_str);
      }
      else if (isa<BinaryOperator>(init)) {
        BinaryOperator * op = cast<BinaryOperator>(init);
	Expr * lhs= op->getLHS()->IgnoreImpCasts();
	if (isa<DeclRefExpr>(lhs)) {
	  struct var_data *init_var;
	  DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
	  query_var (ref->getDecl()->getName().str(), &init_var);
	  analysis_index (op->getRHS()->IgnoreImpCasts(), init_var->min_value_str, 
				init_var->max_value_str);
	}
      }

      BinaryOperator *cond = cast<BinaryOperator>(for_stmt->getCond());
      Expr *lhs = cond->getLHS()->IgnoreImpCasts();
      if (isa<DeclRefExpr>(lhs)) {
	struct var_data *init_var;
	DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
	query_var (ref->getDecl()->getName().str(), &init_var);
	std::string rhs_min, rhs_max;
	analysis_index (cond->getRHS()->IgnoreImpCasts(), rhs_min, rhs_max);
	switch (cond->getOpcode()) {
	  case BO_LT:
	    init_var->max_value_str = "((";
	    init_var->max_value_str += rhs_max;
	    init_var->max_value_str += ")-1)";
	    break;
	  case BO_LE:
	    init_var->max_value_str = "(";
	    init_var->max_value_str += rhs_max;
	    init_var->max_value_str += ")";
	    break;
	  case BO_GT:
	    init_var->min_value_str = "((";
	    init_var->min_value_str += rhs_min;
	    init_var->min_value_str += ")-1)";
	    break;
	  case BO_GE:
	    init_var->min_value_str = "(";
	    init_var->min_value_str += rhs_min;
	    init_var->min_value_str += ")";
	    break;
	  default:
	    break;
	}
      }

      //Record subtask's index and its value range.
      unsigned i = Scope_stack.size();
      if (Scope_stack[i-2].type == 14) {
        k_info.exit_loop = SM->getExpansionLineNumber(s->getLocEnd());

        k_info.replace_line = SM->getExpansionLineNumber(s->getLocStart());

	BinaryOperator *cond = cast<BinaryOperator>(for_stmt->getCond());
	Expr *rhs = cond->getRHS()->IgnoreImpCasts();
	Expr *lhs = cond->getLHS()->IgnoreImpCasts();
	if (isa<DeclRefExpr>(lhs)) {
	  DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
	  k_info.loop_index = ref->getDecl();

	  //Because the loop index may not start with 0.
	  struct var_data *cur_var;
	  query_var (ref->getDecl()->getName().str(), &cur_var);
          k_info.length_var = "(";
          k_info.length_var += cur_var->max_value_str;
          k_info.length_var += "-";
          k_info.length_var += cur_var->min_value_str;
          k_info.length_var += " + 1)";

	  k_info.start_index = cur_var->min_value_str;
  	}
      }
    }

    //Add all variables that be referenced in kernel, but define out of kernel, into kernel parameters.
    if (isa<DeclRefExpr>(s) && process_state == 1) {
      DeclRefExpr * var_ref = cast<DeclRefExpr>(s);

      struct var_data *cur_var;
      unsigned out_kernel = query_var (var_ref->getDecl()->getName().str(), &cur_var);//1 means out of kernel.
      if (cur_var && out_kernel == 1) {
        struct var_decl new_var;
        new_var.type_name = cur_var->type_str;
        new_var.var_name = cur_var->name;
        if (cur_var->type == 0 && cur_var->isKernelArg == false) {
	  cur_var->isKernelArg = true;
          new_var.type = 1;//pointer+fix
          k_info.pointer_parms.push_back(new_var);
        }
        else if (cur_var->type == 1 && cur_var->isKernelArg == false) {
	  cur_var->isKernelArg = true;
          new_var.type = 0;//value+fix
          k_info.val_parms.push_back(new_var);
        }
      }
    }

    //Find alloc function of variables, set its size_str;
    //If the operator is memory access, set its usedByKernel.
    if (isa<BinaryOperator>(s)) {
      BinaryOperator *op = cast<BinaryOperator>(s);
      std::string lhs, rhs;
      std::vector<Expr *> lhs_idx;
      std::vector<Expr *> rhs_idx;
      int kind = check_mem_access (op, lhs, lhs_idx, rhs, rhs_idx);//0: assign.

/*
      #ifdef DEBUG_INFO
      if (kind == 0 && process_state != 1)
        s->dump();
      #endif
*/

      //Calculate algebra operation instructions in kernel.
      if (process_state == 1) {
	if (op->isMultiplicativeOp() || op->isAdditiveOp() || op->isShiftOp()) {
	  k_info.insns ++;
	}
      }

      //Handle stmt: scalar_var = xxx;
      //If scalar_var is first used in kernel, then there is no need to transfer its
      //Value.
      if (process_state == 1 && kind == 0 && lhs.empty()) {
	Expr *lhs = op->getLHS()->IgnoreImpCasts();
        if (isa<DeclRefExpr>(lhs)) {
	  DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
	  struct var_data *cur_var;
	  unsigned out_kernel = query_var (ref->getDecl()->getName().str(), &cur_var);
          if (cur_var && out_kernel == 1 && cur_var->isKernelArg == false) {
            struct var_decl new_var;
            new_var.type_name = cur_var->type_str;
            new_var.var_name = cur_var->name;
            k_info.local_parms.push_back(new_var);
	    cur_var->isKernelArg = true;
	  }
        }
      }

      //Acorrding malloc function, set the size_str for var_data.
      if (kind == 0 && !lhs.empty()) {
        struct var_data *cur_var;
        unsigned out_kernel = query_var (lhs, &cur_var);
	if (cur_var && out_kernel == 1) {
	  if (isa<CStyleCastExpr>(op->getRHS())) {
	    CastExpr * cast_e = cast<CastExpr>(op->getRHS());
	    if (isa<CallExpr>(cast_e->getSubExpr())) {
	      CallExpr *call_e = cast<CallExpr>(cast_e->getSubExpr());
	      Expr * callee = call_e->getCallee()->IgnoreImpCasts();
	      if (isa<DeclRefExpr>(callee)) {
	    	DeclRefExpr *ref = cast<DeclRefExpr>(callee);
		if (ref->getDecl()->getName().str() == "malloc") {
	          std::string str = get_str(call_e->getArg(0)->IgnoreImpCasts());
	          cur_var->size_str = "(";
	          cur_var->size_str += str;
	          cur_var->size_str += ")";
		}
	      }
	    }
	  }
	}
      }

      /*Find which mem buf should transfer back to host.
      if (process_state == 2) {
	if (rhs is writen in kernel)
	  transfer rhs from dev to host.
      }
      */

      //We simple assume: the write mem need transfer from dev to host.
      //The read mem need transfer from host to dev.
      if (process_state == 1) {
        struct var_data *cur_var;
	//lhs is the write to memory.
	//For =/+=/*= assignment operations.
	if (( kind == 0 || kind == 2)  && !lhs.empty()) {
          unsigned out_kernel = query_var (lhs, &cur_var);//1 means out of kernel.
	  cur_var->IdxChains.push_back(lhs_idx);
	  evalue_index (lhs_idx[0], cur_var);
          if (cur_var && out_kernel == 1) {
	    if (cur_var->usedByKernel < 2)
	      cur_var->usedByKernel += 2;
	  }
	}
	//rhs is the read to memory in = assignment operations.
 	if ( kind == 0 && !rhs.empty()) {
          unsigned out_kernel = query_var (rhs, &cur_var);//1 means out of kernel.
	  cur_var->IdxChains.push_back(rhs_idx);
	  evalue_index (rhs_idx[0], cur_var);
          if (cur_var && out_kernel == 1) {
	    if (cur_var->usedByKernel == 0 || cur_var->usedByKernel == 2)
	      cur_var->usedByKernel ++;
	  }
	}
	// lhs and rhs is read memory.
	//For *=/+= or other non-assignment operations.
	if (kind == 1 || kind == 2) {
	  if (!lhs.empty()) {
            unsigned out_kernel = query_var (lhs, &cur_var);//1 means out of kernel.
	    cur_var->IdxChains.push_back(lhs_idx);
	    evalue_index (lhs_idx[0], cur_var);
            if (out_kernel == 1) {
	      if (cur_var->usedByKernel == 0 || cur_var->usedByKernel == 2)
	        cur_var->usedByKernel ++;
	    }
	  }
	  if (!rhs.empty()) {
            unsigned out_kernel = query_var (rhs, &cur_var);//1 means out of kernel.
	    cur_var->IdxChains.push_back(rhs_idx);
	    evalue_index (rhs_idx[0], cur_var);
            if (cur_var && out_kernel == 1) {
	      if (cur_var->usedByKernel == 0 || cur_var->usedByKernel == 2)
	        cur_var->usedByKernel ++;
	    }
	  }
        }
      }
    }

    return true;
  }

  bool VisitVarDecl(VarDecl *d) {
    if (d->getKind() == Decl::Var) {
      Insert_var_data(d, Scope_stack.back());

      //It may also access memory in  declariton expression.
      if (d->hasInit() && process_state == 1) {
        Expr *init_v = d->getInit()->IgnoreImpCasts();
        if (isa<ArraySubscriptExpr>(init_v)) {
          ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(init_v);
	  std::string base_name;
	  std::vector<Expr *> idx_arry;
  	  analysis_array (arr, base_name, idx_arry);
	  struct var_data * cur_var;
          unsigned out_kernel = query_var (base_name, &cur_var);//1 means out of kernel.
	  cur_var->IdxChains.push_back(idx_arry);
	  evalue_index (idx_arry[0], cur_var);
          if (cur_var && out_kernel == 1) {
	    if (cur_var->usedByKernel == 0 || cur_var->usedByKernel == 2)
	      cur_var->usedByKernel ++;
          }
        }
      }
    }
    return true;
  }

  bool VisitFunctionDecl (FunctionDecl *f) {
    struct Scope_data &cur_scope = Scope_stack.back();
    if (f->doesThisDeclarationHaveABody())
      cur_scope.p_func = f;
    return true;
  }

private:
  ASTContext *Ctx;
  SourceManager *SM;
  struct Kernel_Info k_info;
  std::vector<struct Kernel_Info> &k_info_queue;
  std::vector<struct Scope_data> &Scope_stack;
  //0: before kernel; 1: in kernel; 2: after kernel.
  unsigned process_state;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(std::vector<struct Kernel_Info> &k_info_queue, std::vector<struct Scope_data> &s_stack) : Visitor(k_info_queue, s_stack), Scope_stack(s_stack) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
    }
    return true;
  }

  virtual void Initialize(ASTContext &Context) {
    Ctx = &Context;
    Visitor.Initialize (Context);
  }

private:
  MyASTVisitor Visitor;
  ASTContext *Ctx;
  std::vector<struct Scope_data> &Scope_stack;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
private:
  struct Kernel_Info * select_kernel () {
    struct Kernel_Info * ret = NULL;
    unsigned int max_insns = 0;
    unsigned int max_overlap_mems = 0;
    unsigned int max_mems = 0;
    for (auto &k_info : k_info_queue) {
      unsigned int overlap_mems = 0;
      unsigned int mems = 0;
      for (auto &mem_buf : k_info.mem_bufs) {
	//Pre transfer mem 
	if (mem_buf.type & 1) 
	    mems++;
	if (mem_buf.type & 2) 
	  overlap_mems++;
	if (mem_buf.type & 4)
	  overlap_mems++;
	if (mem_buf.type & 8)
	  mems++;
      }
      mems += overlap_mems;

      if (k_info.insns > max_insns) {
	max_insns = k_info.insns;
	ret = &k_info;
      }
    }
    return ret;
  }
public:
  MyFrontendAction() {}

//Callback at the end of processing a single input.
  void EndSourceFileAction() override {

    struct Kernel_Info * k_info = select_kernel ();
    if (k_info == NULL)
      return;

    generator.set_enter_loop(k_info->enter_loop);
    generator.set_exit_loop(k_info->exit_loop);
    generator.set_init_cite(k_info->init_cite);
    generator.set_finish_cite (k_info->finish_cite);
    generator.set_create_mem_cite (k_info->create_mem_cite);

    struct var_decl var;
    var.type_name = "int";
    var.var_name = "start_index";
    var.type = 2;
    generator.add_kernel_arg(var);

    var.var_name = "end_index";
    generator.add_kernel_arg(var);

    generator.set_length_var(k_info->length_var);
    generator.set_start_index(k_info->start_index);
    generator.set_replace_line(k_info->replace_line);

    for (unsigned i = 0; i < k_info->val_parms.size(); i++)
    {
      generator.add_kernel_arg(k_info->val_parms[i]);
    }
    for (unsigned i = 0; i < k_info->pointer_parms.size(); i++)
    {
      generator.add_kernel_arg(k_info->pointer_parms[i]);
    }
    for (unsigned i = 0; i < k_info->local_parms.size(); i++)
    {
      generator.add_local_var(k_info->local_parms[i]);
    }
    for (unsigned i = 0; i < k_info->mem_bufs.size(); i++)
    {
      generator.add_mem_xfer(k_info->mem_bufs[i]);
    }

    generator.set_logical_streams (2);
    generator.set_task_blocks (4);

    generator.generateDevFile();
    generator.generateHostFile();
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    std::string f_name = file.str();
    int f_index = f_name.find_last_of('/');
    f_name = f_name.substr(f_index+1,-1);
    generator.set_InputFile (f_name);

    struct Scope_data new_scope;
    new_scope.type = 0;
    new_scope.p_func = NULL;
    Scope_stack.push_back (new_scope);

    return llvm::make_unique<MyASTConsumer>(k_info_queue, Scope_stack);
  }

private:
  WriteInFile generator;
  std::vector<struct Kernel_Info> k_info_queue;
  std::vector<struct Scope_data> Scope_stack;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
