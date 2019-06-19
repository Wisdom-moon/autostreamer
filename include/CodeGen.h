//===------------------------ CodeGen.h --------------------------===//
//
// This file is distributed under the Universidade Federal de Minas Gerais -
// UFMG Open Source License. See LICENSE.TXT for details.
//
// Copyright (C) 2018  Peng Zhang 
//
//===----------------------------------------------------------------------===//
//
// CodeGen: according the instructions, generate Streamed pragrom from
// C code.
//
//===----------------------------------------------------------------------===//
#ifndef CODEGEN_H
#define CODEGEN_H

#include <vector>
#include <string>

#include "KernelInfo.h"

class CodeGen {

  private:

  //===---------------------------------------------------------------------===
  //                             Names 
  //===---------------------------------------------------------------------===
  std::string InputFile;
  std::string DevFile;
  std::string HostFile;
  std::string KernelName;
  std::string DevLibName;

  //===---------------------------------------------------------------------===
  //                            Parameters for hStreams generator 
  //===---------------------------------------------------------------------===
  unsigned kernel_args;
  unsigned val_num;
  unsigned pointer_num;
  std::vector<var_decl> arg_decls;//all of kernel args.
  std::vector<var_decl> var_decls;//local define vars in kernel.
  std::vector<var_decl> fix_decls;//args that independ to streams and tasks.

  unsigned logical_streams;
  unsigned task_blocks;
  //The total task size.
  std::string loop_var;
  std::string length_var_name;
  std::string start_index_str;
  //mem xfers that is before all kernel execution.
  std::vector<mem_xfer> pre_xfers;
  //all mem bufs that need be created.
  std::vector<mem_xfer> mem_bufs;
  //xfers that from host to device.
  std::vector<mem_xfer> h2d_xfers;
  //xfers that from device to host.
  std::vector<mem_xfer> d2h_xfers;
  //mem xfers that is transfer after all kernel execution.
  std::vector<mem_xfer> post_xfers;

  unsigned replace_line;
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_site;
  unsigned finish_site;
  unsigned create_mem_site;

  unsigned include_insert_site;
  unsigned cuda_kernel_site;
  //===---------------------------------------------------------------------===
  //                           Utility functions 
  //===---------------------------------------------------------------------===
  bool is_include (std::string line);
  bool in_loop (unsigned LineNo);
  std::string generateCUKernelCode();

  public:

  CodeGen() ;
  ~CodeGen() ;
  // To print Information in source file.
  void generateDevFile ();
  void generateHostFile ();
  void set_InputFile(std::string input);
  void set_enter_loop (unsigned LineNo);
  void set_exit_loop (unsigned LineNo);
  void set_init_site (unsigned LineNo);
  void set_finish_site (unsigned LineNo);
  void set_create_mem_site (unsigned LineNo);
  void add_kernel_arg(struct var_decl var);
  void add_local_var(struct var_decl var);
  void set_replace_line(unsigned n);
  void set_logical_streams(unsigned n);
  void set_task_blocks(unsigned n);
  void set_length_var(std::string name);
  void set_loop_var(std::string name);
  void set_start_index(std::string name);
  void add_mem_xfer(struct mem_xfer m);
  void set_include_site (unsigned LineNo);
  void set_cuda_site (unsigned LineNo);
  
  void generateOCLDevFile ();
  void generateOCLHostFile ();

  void generateCUDAFile ();
};

//===------------------------ CodeGen.h --------------------------===//
#endif
