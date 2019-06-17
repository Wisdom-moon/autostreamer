//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "rewriter.h"

//#define DEBUG_INFO
#define GEN_CUDA
//#define GEN_OCL

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.

struct File_info {
  int last_include = 0;
  int start_function = 0;
  int return_site = 0;
} f_info;

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(std::vector<struct Kernel_Info> &k_info_queue, std::vector<struct Scope_data> &s_stack) : Visitor(k_info_queue, s_stack), Scope_stack(s_stack) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      if (SM.isWrittenInMainFile((*b)->getLocation()))
        Analyser.TraverseDecl(*b);
    }

    //Construct FunctionInfo and LoopInfo.
    std::vector<FunctionInfo *> funcList;
    for (int i = 0; i < TopScope->children.size(); i++) {
      ScopeIR * fileScope = TopScope->children[i];
      for (int j = 0; j < fileScope->children.size(); j++) {
	ScopeIR * funcScope = fileScope->children[j];
	if (funcScope->type == 1) {
	  FunctionInfo *funcInfo = new FunctionInfo(funcScope);
	  funcInfo->rootLoop = new LoopInfo(funcScope);
	  funcInfo->rootLoop->buildLoopTree();

	  funcList.push_back(funcInfo);
	}
      }
    }


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
    generator.set_init_site(k_info->init_site);
    generator.set_create_mem_site (k_info->create_mem_site);
    generator.set_finish_site (f_info.return_site);
    generator.set_include_site (f_info.last_include);
    generator.set_cuda_site (f_info.start_function);

    struct var_decl var;
#ifndef GEN_OCL
    var.type_name = "int";
    var.var_name = "start_index";
    var.type = 2;
    generator.add_kernel_arg(var);

    var.var_name = "end_index";
    generator.add_kernel_arg(var);
#endif

    generator.set_length_var(k_info->length_var);
    generator.set_loop_var(k_info->loop_var);
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

    //Generate OpenCL Code
#ifdef GEN_OCL
    generator.generateOCLDevFile();
    generator.generateOCLHostFile();
    return;
#endif

    //Generate CUDA Code
#ifdef GEN_CUDA
    generator.generateCUDAFile();
    return;
#endif

    //Generate hStreams Code by default
    generator.generateDevFile();
    generator.generateHostFile();
    return;
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
    new_scope.process_state = 0;
    Scope_stack.push_back (new_scope);

    f_info.last_include = -1;
    f_info.start_function = -1;

    return llvm::make_unique<MyASTConsumer>(k_info_queue, Scope_stack);
  }

private:
  WriteInFile generator;
  std::vector<struct Kernel_Info> k_info_queue;
  std::vector<struct Scope_data> Scope_stack;
};
