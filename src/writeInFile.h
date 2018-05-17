//===------------------------ writeInFile.h --------------------------===//
//
// This file is distributed under the Universidade Federal de Minas Gerais -
// UFMG Open Source License. See LICENSE.TXT for details.
//
// Copyright (C) 2018  Peng Zhang 
//
//===----------------------------------------------------------------------===//
//
// WriteInFile: according the instructions, generate hStreams pragrom from
// OpenMP program.
//
//===----------------------------------------------------------------------===//
#ifndef WRITEINFILE_H
#define WRITEINFILE_H

#include <vector>
#include <string>

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

class WriteInFile {

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
  std::string length_var_name;
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
  unsigned init_cite ;
  unsigned finish_cite;

  //===---------------------------------------------------------------------===
  //                           Utility functions 
  //===---------------------------------------------------------------------===
  bool is_include (std::string line);
  bool in_loop (unsigned LineNo);

  public:

  WriteInFile() ;
  ~WriteInFile() ;
  // To print Information in source file.
  void generateDevFile ();
  void generateHostFile ();
  void set_InputFile(std::string input);
  void set_enter_loop (unsigned LineNo);
  void set_exit_loop (unsigned LineNo);
  void set_init_cite (unsigned LineNo);
  void set_finish_cite (unsigned LineNo);
  void add_kernel_arg(struct var_decl var);
  void add_local_var(struct var_decl var);
  void set_replace_line(unsigned n);
  void set_logical_streams(unsigned n);
  void set_task_blocks(unsigned n);
  void set_length_var(std::string name);
  void add_mem_xfer(struct mem_xfer m);
  
};

//===------------------------ writeInFIle.h --------------------------===//
#endif
