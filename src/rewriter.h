//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef REWRITER_H
#define REWRITER_H

#include <sstream>
#include <string>
#include <iostream>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

#include "writeInFile.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

struct var_data {
  std::string name;
  std::string type_str;
  std::string size_str;
  //0: pointer, 1: value;
  unsigned int type;
  //For pointer, min_value is index's min value
  //For non-pointer, min_value is variable's min value
  std::string min_value_str;
  //max_value is assemble to min_value;
  std::string max_value_str;
  //0: not used in kernel; 1: read only in kernel; 
  //2: write only in kernel; 3:r+w in kernel;
  //This attr is only used by pointer.
  unsigned int usedByKernel;
  bool isKernelArg;
  //For memory access in kernel, its Index expr chain.
  std::vector<std::vector<Expr *>> IdxChains;
  //Set all variables = 3, compute the value of index.
  int value_min;
  int value_max;
  //Its declare stmt.
  Decl * decl_stmt;
};

struct Kernel_Info {
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_cite ;
  unsigned finish_cite;
  unsigned create_mem_cite;
  unsigned replace_line;
  std::vector<mem_xfer> mem_bufs;
  std::vector<var_decl> val_parms;
  std::vector<var_decl> pointer_parms;
  std::vector<var_decl> local_parms;
  std::string length_var;
  std::string loop_var;

  //The omp parallel for iteration index variable declaration
  ValueDecl * loop_index;
  //The compute instructions.
  unsigned int insns;
  //The init value of loop index may not be 0!
  std::string start_index;
};

struct Scope_data {
  //Start line and colum Number;
  unsigned int sline, scol;
  //End line and colum Number.
  unsigned int eline, ecol;
  //0: file global; 1: function; 2: for loop; 3: do loop; 4: while loop;
  //5: if stmt; 6: switch stmt; 7: switch case;
  //8: Try; 9: Catch; 10: SEH; 11: compound; 12: others.
  //13: omp parallel for; 14: captured stmt;
  unsigned int type;

  //varables table.
  std::map<std::string, struct var_data> var_table;
  FunctionDecl * p_func;
};
#endif
