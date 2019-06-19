//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef KERNELINFO_H
#define KERNELINFO_H

#include <sstream>
#include <string>
#include <iostream>

#include "ClangHeader.h"

struct var_decl {
  std::string type_name;
  std::string var_name;
  unsigned id;//the order in kernel args.
  unsigned type;//00=value + fix, 01=point+fix, 10=value+var, 11=p+v.
};

struct mem_xfer {
  std::string  buf_name;
  std::string size_string;
  std::string type_name;
  std::string elem_type;
  unsigned dim;//The dim of arry;
  unsigned type;//x1:pre_xfers; x1x:h2d; 1xx:d2h; 1xxx: post_xfer
};

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
  //0: enter a new function body; 1: in kernel; 2: after kernel but in same function.
  int process_state;
};

struct File_Info {
  int last_include = 0;
  int start_function = 0;
  int return_site = 0;
};

struct Kernel_Info {
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_site ;
  unsigned finish_site;
  unsigned create_mem_site;
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

#endif
