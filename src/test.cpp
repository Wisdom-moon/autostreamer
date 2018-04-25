#include <iostream>
#include "writeInFile.h"

int main (int argc, char **argv) {
  WriteInFile test;

  if (argc < 2) {
    std::cout << "Usage: test omp_file\n";
    return -1;
  }

  test.set_InputFile (argv[1]);
  test.set_enter_loop (30);
  test.set_exit_loop (34);
  test.set_init_cite (29);
  test.set_finish_cite (35);

  struct var_decl var;
  var.type_name = "int";
  var.var_name = "start_index";
  var.id = 0;
  var.type = 2;
  test.add_kernel_arg(var);

  var.var_name = "end_index";
  var.id = 1;
  test.add_kernel_arg(var);

  var.type_name = "float *";
  var.var_name = "hostInput1";
  var.id = 2;
  var.type = 1;
  test.add_kernel_arg(var);

  var.var_name = "hostInput2";
  var.id = 3;
  test.add_kernel_arg(var);

  var.var_name = "hostOutput";
  var.id = 4;
  test.add_kernel_arg(var);

  struct replace_info r;
  r.start_num = 12;
  r.size = 1;
  r.name = "start_index";
  r.line_no = 31;
  test.add_replace_info(r);
  r.start_num = 17;
  r.size = 11;
  r.name = "end_index";
  test.add_replace_info(r);

  test.set_logical_streams (2);
  test.set_task_blocks (4);
  test.set_length_var("inputLength");

  struct mem_xfer m;
  m.buf_name = "hostInput1";
  m.size_string = "inputLengthBytes";
  m.type_name = "float";
  m.type = 2;
  test.add_mem_xfer (m);
  m.buf_name = "hostInput2";
  test.add_mem_xfer (m);
  m.buf_name = "hostOutput";
  m.type = 4;
  test.add_mem_xfer (m);
  
  test.generateDevFile();
  test.generateHostFile ();
}
