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
  //0: not used in kernel; 1: read only in kernel; 
  //2: write only in kernel; 3:r+w in kernel;
  unsigned int usedByKernel;
  bool isKernelArg;
};

struct Kernel_Info {
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_cite ;
  unsigned finish_cite;
  std::vector<mem_xfer> mem_bufs;
  std::vector<replace_info> replace_vars;
  std::vector<var_decl> val_parms;
  std::vector<var_decl> pointer_parms;
  std::string length_var;
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
