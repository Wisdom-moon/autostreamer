//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "rewriter.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
private:
  //Check memory read/write, return var name.
  //write name only occurs when op is "=", and lvalue is the write memory.
  //All other mem access is read.
  //return 0: is a assignment opt. 1: others.
  int check_mem_access (BinaryOperator *op, std::string &lhs, std::string &rhs) {
    int ret = 1;
    if (op->isAssignmentOp()) {
      ret = 0;
    }
 
    if (isa<ArraySubscriptExpr>(op->getRHS()->IgnoreImpCasts())) {
      ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(op->getRHS()->IgnoreImpCasts());
      Expr *base = arr->getBase()->IgnoreImpCasts();
      if (isa<DeclRefExpr>(base)) {
        DeclRefExpr *ref = cast<DeclRefExpr>(base);
        rhs = ref->getDecl()->getName().str();
      }
    }
    else if (isa<DeclRefExpr>(op->getRHS()->IgnoreImpCasts())) {
      DeclRefExpr *ref = cast<DeclRefExpr>(op->getRHS()->IgnoreImpCasts());
      struct var_data *cur_var;
      query_var (ref->getDecl()->getName().str(), &cur_var);
      if (cur_var && cur_var->type == 0)
        rhs = ref->getDecl()->getName().str();
    }

    if (isa<ArraySubscriptExpr>(op->getLHS()->IgnoreImpCasts())) {
      ArraySubscriptExpr * arr = cast<ArraySubscriptExpr>(op->getLHS()->IgnoreImpCasts());
      Expr *base = arr->getBase()->IgnoreImpCasts();
      if (isa<DeclRefExpr>(base)) {
        DeclRefExpr *ref = cast<DeclRefExpr>(base);
        lhs = ref->getDecl()->getName().str();
      }
    }
    else if (isa<DeclRefExpr>(op->getLHS()->IgnoreImpCasts())) {
      DeclRefExpr *ref = cast<DeclRefExpr>(op->getLHS()->IgnoreImpCasts());
      struct var_data *cur_var;
      query_var (ref->getDecl()->getName().str(), &cur_var);
      if (cur_var && cur_var->type == 0)
        lhs = ref->getDecl()->getName().str();
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
      std::string str1 = SM->getCharacterData(s->getLocStart());
      std::string str2 = SM->getCharacterData(s->getLocEnd());
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

  struct var_data CreateVar (VarDecl *d) {
    struct var_data new_var;
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
    return new_var;
  }


public:
  MyASTVisitor(struct Kernel_Info &k, std::vector<struct Scope_data> &s_stack): k_info(k), Scope_stack(s_stack)  {
    process_state = 0;
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
      new_scope.sline = SM->getSpellingLineNumber(st->getLocStart());
      new_scope.scol = SM->getSpellingColumnNumber(st->getLocStart());
      new_scope.eline = SM->getSpellingLineNumber(st->getLocEnd());
      new_scope.ecol = SM->getSpellingColumnNumber(st->getLocEnd());

    if (isa<CompoundStmt>(st) || isa<WhileStmt>(st) || isa<CXXCatchStmt>(st)
       || isa<CXXForRangeStmt>(st) || isa<CXXTryStmt>(st) || isa<DoStmt>(st)
       || isa<ForStmt>(st) || isa<IfStmt>(st) || isa<SEHExceptStmt>(st)
       || isa<SEHFinallyStmt>(st) || isa<SEHFinallyStmt>(st)
       || isa<SwitchCase>(st) || isa<SwitchStmt>(st) || isa<WhileStmt>(st)
       || isa<CapturedStmt>(st)) {
      if (isa<CompoundStmt>(st)) {
	if (cur_scope.p_func) {
	  new_scope.type = 1;
	  //Add ParmVarDecl to new_scope.var_table
	  for (unsigned i = 0; i < cur_scope.p_func->getNumParams(); i++) {
            struct var_data new_var = CreateVar (cur_scope.p_func->getParamDecl(i));
            new_scope.var_table.insert({new_var.name, new_var});
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
      k_info.enter_loop = SM->getSpellingLineNumber(st->getLocStart());
      k_info.init_cite = k_info.enter_loop;
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
	  for (auto &scope : Scope_stack) {
	    for (auto &val_pair : scope.var_table) {
	      struct var_data cur_var = val_pair.second;
	      new_mem.buf_name = cur_var.name;
	      new_mem.size_string = cur_var.size_str;
	      new_mem.type_name = cur_var.type_str;
	      switch (cur_var.usedByKernel) {
		case 1:
	  	  new_mem.type = 3;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		case 2:
	  	  new_mem.type = 5;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		case 3:
	  	  new_mem.type = 7;
	   	  k_info.mem_bufs.push_back(new_mem);
		  break;
		default:
		  break;
	      }
	    }
	  }

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
      unsigned i = Scope_stack.size();
      if (Scope_stack[i-2].type == 14) {
        k_info.exit_loop = SM->getSpellingLineNumber(s->getLocEnd());
        k_info.finish_cite = k_info.exit_loop;

	struct replace_info new_rep;
        ForStmt *for_stmt = cast<ForStmt>(s);
	Stmt *init = for_stmt->getInit();
	if (isa<DeclStmt>(init)) {
	  VarDecl *var = cast<VarDecl>(cast<DeclStmt>(init)->getSingleDecl());
	  Expr * init_val = var->getInit();
	  new_rep.start_num = SM->getSpellingColumnNumber(init_val->getLocStart()) - 1;
	  new_rep.size = 1;
	  new_rep.name = "start_index";
	  new_rep.line_no = SM->getSpellingLineNumber(init_val->getLocStart());
	  k_info.replace_vars.push_back(new_rep);
	}

	BinaryOperator *cond = cast<BinaryOperator>(for_stmt->getCond());
	Expr *rhs = cond->getRHS()->IgnoreImpCasts();
	if (isa<DeclRefExpr>(rhs)) {
	  DeclRefExpr *ref = cast<DeclRefExpr>(rhs);
          k_info.length_var = ref->getDecl()->getName().str();
	  new_rep.start_num = SM->getSpellingColumnNumber(ref->getLocStart()) - 1;
	  new_rep.size = k_info.length_var.size();
	  new_rep.name = "end_index";
	  new_rep.line_no = SM->getSpellingLineNumber(ref->getLocStart());
	  k_info.replace_vars.push_back(new_rep);
  	}
      }
    }

    //Add all variables that be referenced in kernel, but define out of kernel, into kernel parameters.
    if (isa<DeclRefExpr>(s) && process_state == 1) {
      DeclRefExpr * var_ref = cast<DeclRefExpr>(s);

      struct var_data *cur_var;
      unsigned in_kernel = query_var (var_ref->getDecl()->getName().str(), &cur_var);//1 means out of kernel.
      if (cur_var && in_kernel == 1) {
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
      int kind = check_mem_access (op, lhs, rhs);//0: assign.

      //Acorrding malloc function, set the size_str for var_data.
      if (kind == 0 && !lhs.empty()) {
        struct var_data *cur_var;
        unsigned in_kernel = query_var (lhs, &cur_var);
	if (cur_var && in_kernel == 1) {
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
	if (kind == 0 && !lhs.empty()) {
          unsigned in_kernel = query_var (lhs, &cur_var);//1 means out of kernel.
          if (cur_var && in_kernel == 1) {
	    if (cur_var->usedByKernel < 2)
	      cur_var->usedByKernel += 2;
	  }
	}
	// lhs and rhs is read memory.
	else {
	  if (!lhs.empty()) {
            unsigned in_kernel = query_var (lhs, &cur_var);//1 means out of kernel.
            if (cur_var && in_kernel == 1) {
	      if (cur_var->usedByKernel == 0 || cur_var->usedByKernel == 2)
	        cur_var->usedByKernel ++;
	    }
	  }
	  if (!rhs.empty()) {
            unsigned in_kernel = query_var (rhs, &cur_var);//1 means out of kernel.
            if (cur_var && in_kernel == 1) {
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
      struct Scope_data &cur_scope = Scope_stack.back();
      struct var_data new_var = CreateVar (d);
      cur_scope.var_table.insert({new_var.name, new_var});
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
  struct Kernel_Info &k_info;
  std::vector<struct Scope_data> &Scope_stack;
  //0: before kernel; 1: in kernel; 2: after kernel.
  unsigned process_state;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(struct Kernel_Info &k_info, std::vector<struct Scope_data> &s_stack) : Visitor(k_info, s_stack), Scope_stack(s_stack) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
      if (Ctx->getSourceManager().isWrittenInMainFile((*b)->getLocation()))
        (*b)->dump();
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
public:
  MyFrontendAction() {}

//Callback at the end of processing a single input.
  void EndSourceFileAction() override {
    generator.set_enter_loop(k_info.enter_loop);
    generator.set_exit_loop(k_info.exit_loop);
    generator.set_init_cite(k_info.init_cite);
    generator.set_finish_cite (k_info.finish_cite);

    struct var_decl var;
    var.type_name = "int";
    var.var_name = "start_index";
    var.type = 2;
    generator.add_kernel_arg(var);

    var.var_name = "end_index";
    generator.add_kernel_arg(var);

    generator.set_length_var(k_info.length_var);
    for (unsigned i = 0; i < k_info.val_parms.size(); i++)
    {
      generator.add_kernel_arg(k_info.val_parms[i]);
    }
    for (unsigned i = 0; i < k_info.pointer_parms.size(); i++)
    {
      generator.add_kernel_arg(k_info.pointer_parms[i]);
    }
    for (unsigned i = 0; i < k_info.replace_vars.size(); i++)
    {
      generator.add_replace_info(k_info.replace_vars[i]);
    }
    for (unsigned i = 0; i < k_info.mem_bufs.size(); i++)
    {
      generator.add_mem_xfer(k_info.mem_bufs[i]);
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

    return llvm::make_unique<MyASTConsumer>(k_info, Scope_stack);
  }

private:
  WriteInFile generator;
  struct Kernel_Info k_info;
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
