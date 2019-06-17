//===------------------------ CodeGen.cpp --------------------------===//
//
//
// This file is distributed under the Universidade Federal de Minas Gerais -
// UFMG Open Source License. See LICENSE.TXT for details.
//
// Copyright (C) 2018  Peng Zhang (pengzhang_nudt@sina.com)
//
//===----------------------------------------------------------------------===//
//
// 
//===----------------------------------------------------------------------===//
#include <fstream>
#include <iostream>
#include <sstream>


#include "CodeGen.h" 

#define CarriageReturn 13

using namespace std;

#define DEBUG_TYPE "CodeGen"


void CodeGen::generateDevFile() {
  if (enter_loop <= 0)
  return;

fstream Infile(InputFile.c_str());
if (!Infile){
  cerr << "\nError. File " << InputFile << " has not found.\n";
  return;
}
std::string Line = std::string();
ofstream File(DevFile.c_str());
cerr << "\nWriting output to dev file " << DevFile<< "\n";

unsigned LineNo = 1;
while (!Infile.eof()) {
  Line = std::string();
  std::getline(Infile, Line);

  if (replace_line == LineNo) {
    std::string::size_type start_i = Line.find("for");
    if (start_i != std::string::npos) {
      std::string::size_type init_begin, init_end, cond_begin, cond_end;
      init_begin = Line.find("=", start_i) + 1;
      init_end = Line.find(";", init_begin);
      cond_begin = Line.find("<", init_end) + 1;
      cond_end = Line.find(";", cond_begin);

      Line.replace(cond_begin, cond_end - cond_begin, " end_index");
      Line.replace(init_begin, init_end - init_begin, " start_index");
    }

    File << Line << "\n";
  }
  else if (is_include (Line) || in_loop(LineNo)) {
    File << Line << "\n";
  }
  if (enter_loop == LineNo) {
    // Add hStreams Include fine.
    File << "#include <intel-coi/sink/COIPipeline_sink.h>\n\n";

    //Add kernel function decl.
    File << "COINATIVELIBEXPORT void\n"
	<< KernelName <<" ( uint64_t arg0";
    //Add kernel args.
    for (unsigned i = 1; i < kernel_args; i++) {
      File << ",\n\tuint64_t arg" << i;
    }
    File <<"\n)\n{\n\n";

    //Add arguments decl.
    for (unsigned i = 0; i < arg_decls.size(); i++) {
      std::string decl_str = arg_decls[i].type_name;
      std::string::size_type idx = decl_str.find("*");
      if (idx == std::string::npos) {
        decl_str += " ";
	decl_str += arg_decls[i].var_name;
      }
      else {
        //Handle for mult-dim array, its decl should be TYPE (*name)[nn]
	decl_str.insert(idx+1, arg_decls[i].var_name);
      }

      std::string type_str = arg_decls[i].type_name;
      std::string::size_type t_idx = type_str.find("const");
      if (t_idx != std::string::npos)
        type_str.erase (t_idx, 5);
      t_idx = type_str.find("double");
      if (t_idx == std::string::npos)
        t_idx = type_str.find("float");
      //Handle double/float scalar parameters, its conversion expression
      //should be like this: TYPE a = *((TYPE *) (&arg));
      if (idx == std::string::npos && t_idx != std::string::npos) {
        File <<"  "<<decl_str
	  <<" = *((" <<type_str
	  <<" *) (&arg" <<arg_decls[i].id <<"));\n";
      }
      else {
        File <<"  "<<decl_str
	  <<" = (" <<type_str
	  <<") arg" <<arg_decls[i].id <<";\n";
      }
    }
    //Add local variables decl.
    for (unsigned i = 0; i < var_decls.size(); i++) {
      File <<"  "<<var_decls[i].type_name
	<<" "<<var_decls[i].var_name<<";\n";
    }

    File << Line <<"\n";
  }
  if (exit_loop == LineNo) {
    File << Line <<"\n}";
  }

  LineNo++;
}

File.close();
}

void CodeGen::generateHostFile() {
  if (enter_loop <= 0)
  return;

  fstream Infile(InputFile.c_str());
  if (!Infile){
    cerr << "\nError. File " << InputFile << " has not found.\n";
    return;
  }
  std::string Line = std::string();
  ofstream File(HostFile.c_str());
  cerr << "\nWriting output to host file " << HostFile<< "\n";

  //Add hStreams include file;
  File << "#include <hStreams_source.h>\n#include <hStreams_app_api.h>\n"
       << "#include <intel-coi/common/COIMacros_common.h>\n";

  unsigned LineNo = 1;
  while (!Infile.eof()) {
    Line = std::string();
    std::getline(Infile, Line);

  if (init_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams init code
    File <<Start<<"uint32_t logical_streams_per_place= 1;\n"
         <<Start<<"uint32_t places_per_domain = "<<logical_streams<<";\n"
	 <<Start<<"HSTR_OPTIONS hstreams_options;\n\n"
	 <<Start<<"hStreams_GetCurrentOptions(&hstreams_options, sizeof(hstreams_options));\n"
	 <<Start<<"hstreams_options.verbose = 0;\n"
	 <<Start<<"hstreams_options.phys_domains_limit = 256;\n"
	 <<Start<<"char *libNames[20] = {NULL,NULL};\n"
	 <<Start<<"unsigned int libNameCnt = 0;\n"
	 <<Start<<"libNames[libNameCnt++] = \""<<DevLibName<<"\";\n"
	 <<Start<<"hstreams_options.libNames = libNames;\n"
	 <<Start<<"hstreams_options.libNameCnt = (uint16_t)libNameCnt;\n"
	 <<Start<<"hStreams_SetOptions(&hstreams_options);\n\n"
	 <<Start<<"int iret = hStreams_app_init(places_per_domain, logical_streams_per_place);\n"
	 <<Start<<"if( iret != 0 )\n"
	 <<Start<<"{\n"
	 <<Start<<"  printf(\"hstreams_app_init failed!\\n\");\n"
	 <<Start<<"  exit(-1);\n"
	 <<Start<<"}\n\n";
  }
  if (create_mem_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams buf create code
    for (unsigned i = 0; i < mem_bufs.size(); i++) {
      File <<Start<<"(hStreams_app_create_buf("
	   <<"("<<mem_bufs[i].type_name<<")"
	<<mem_bufs[i].buf_name<<", "<<mem_bufs[i].size_string<<"));\n";
    }
  }
  if (enter_loop == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams mem transfer code
    for (unsigned i = 0;i < pre_xfers.size(); i++) {
      File <<Start<<"(hStreams_app_xfer_memory("
	   <<"("<<pre_xfers[i].type_name<<")"
	   <<pre_xfers[i].buf_name<<", "
	   <<"("<<pre_xfers[i].type_name<<")"
	   <<pre_xfers[i].buf_name<<" ,"
	   <<pre_xfers[i].size_string<<", 0, HSTR_SRC_TO_SINK, NULL));\n";
    }

    //Add hStreams multStreams variables decl
    File <<Start<<"int sub_blocks = "<<length_var_name<<"/ "<<task_blocks<<";\n"
	 <<Start<<"int remain_index = "<<length_var_name<<"% "<<task_blocks<<";\n"
	 <<Start<<"int start_index = 0;\n"
	 <<Start<<"int end_index = 0;\n";

    //Add hStreams kernel arguments set code
    File <<Start<<"uint64_t args["<<kernel_args<<"];\n";
    for (unsigned i = 0; i < fix_decls.size(); i++) {
      std::string::size_type idx = fix_decls[i].type_name.find("*");
      if (idx == std::string::npos) {
        idx = fix_decls[i].type_name.find("double");
        if (idx == std::string::npos)
	  idx = fix_decls[i].type_name.find("float");
      } else
        idx = std::string::npos;

      File <<Start<<"args["<<fix_decls[i].id<<"] = (uint64_t) ";
      if (idx != std::string::npos)
        File <<"*((uint64_t *) (&"<<fix_decls[i].var_name<<"));\n";
      else
        File <<fix_decls[i].var_name<<";\n";
    }

    //Add hStreams synchronize code
    File <<Start<<"hStreams_ThreadSynchronize();\n";

    //Init start_index.
    File <<Start<<"start_index = "<<start_index_str<<";\n";

    //Use idx_subtask is to avoid be the same to original variable name
    File <<Start<<"for (int idx_subtask = 0; idx_subtask < "<<task_blocks<<"; idx_subtask++)\n"
	 <<Start<<"{\n"
	 <<Start<<"  args[0] = (uint64_t) start_index;\n"
	 <<Start<<"  end_index = start_index + sub_blocks;\n"
	 <<Start<<"  if (idx_subtask < remain_index)\n"
	 <<Start<<"    end_index ++;\n"
	 <<Start<<"  args[1] = (uint64_t) end_index;\n";
    for (unsigned i = 0;i < h2d_xfers.size(); i++) {
      File <<Start<<"  (hStreams_app_xfer_memory(&"
	   <<h2d_xfers[i].buf_name<<"[start_index]";
      for (unsigned j = 1; j < h2d_xfers[i].dim; j++)
	File <<"[0]";
      File <<", &"<<h2d_xfers[i].buf_name<<"[start_index]";
      for (unsigned j = 1; j < h2d_xfers[i].dim; j++)
	File <<"[0]";
      File <<", (end_index - start_index) * sizeof ("<<h2d_xfers[i].elem_type<<"), "
	   <<"idx_subtask \% "<<logical_streams<<", HSTR_SRC_TO_SINK, NULL));\n";
    }

    File <<Start<<"  (hStreams_EnqueueCompute(\n"
	 <<Start<<"			idx_subtask % "<<logical_streams<<",\n"
	 <<Start<<"			\""<<KernelName<<"\",\n"
	 <<Start<<"			"<<val_num<<",\n"
	 <<Start<<"			"<<pointer_num<<",\n"
	 <<Start<<"			args,\n"
	 <<Start<<"			NULL,NULL,0));\n";

    for (unsigned i = 0;i < d2h_xfers.size(); i++) {
      File <<Start<<"  (hStreams_app_xfer_memory(&"
	   <<d2h_xfers[i].buf_name<<"[start_index]";
      for (unsigned j = 1; j < d2h_xfers[i].dim; j++)
	File <<"[0]";
      File <<", &"<<d2h_xfers[i].buf_name<<"[start_index]";
      for (unsigned j = 1; j < d2h_xfers[i].dim; j++)
	File <<"[0]";
      File <<", (end_index - start_index) * sizeof ("<<d2h_xfers[i].elem_type<<"), "
	   <<"idx_subtask \% "<<logical_streams<<", HSTR_SINK_TO_SRC, NULL));\n";
    }

    File <<Start<<"  start_index = end_index;\n"
         <<Start<<"}\n";

    File<<Start<<"hStreams_ThreadSynchronize();\n";
    for (auto &mem_xfer : post_xfers) {
      File <<Start<<"  (hStreams_app_xfer_memory("
	   <<"("<<mem_xfer.type_name<<")"
	   <<mem_xfer.buf_name<<", "
	   <<"("<<mem_xfer.type_name<<")"
	   <<mem_xfer.buf_name<<" ,"
	   <<mem_xfer.size_string<<", 0, HSTR_SINK_TO_SRC, NULL));\n";
    }
    if (post_xfers.size() > 0)
      File<<Start<<"hStreams_ThreadSynchronize();\n";

    LineNo++;
    continue;
  }
  if (in_loop(LineNo)) {
    LineNo++;
    continue;
  }
  if (finish_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }

    File<<Start<<"hStreams_app_fini();\n";
  }
  if (exit_loop == LineNo) {
    LineNo++;
    continue;
  }

  File << Line <<"\n";
  LineNo++;
  }

  File.close();
}

CodeGen::CodeGen() {
  kernel_args = 0;
  val_num = 0;
  pointer_num = 0;
  logical_streams = 0;
  task_blocks = 0;
  replace_line = 0;
  enter_loop = 0;
  exit_loop = 0;
  init_site = 0;
  finish_site = 0;
}

CodeGen::~CodeGen() {
}

void CodeGen::set_InputFile(std::string inputFile) {
  std::string key (".");
  std::size_t found = inputFile.rfind(key);

  InputFile = inputFile;

  DevFile = inputFile;
  DevFile.replace(found, key.length(), "_dev.");

  HostFile = inputFile;
  HostFile.replace(found, key.length(), "_host.");

  DevLibName = "kernel.so";
  KernelName = "kernel";
}

bool CodeGen::is_include (std::string line) {
  std::size_t found1 = line.find_first_not_of(" ");
  std::size_t found2 = line.find_first_not_of("\t");
  std::size_t found = found1 > found2 ? found1 : found2;
  if (found < line.size())
    return (line.compare(found, 8, "#include") == 0);
  else
    return false;
}

bool CodeGen::in_loop (unsigned LineNo) {
  return (LineNo > enter_loop && LineNo < exit_loop);
}

void CodeGen::set_enter_loop(unsigned LineNo) {
  enter_loop = LineNo;
}

void CodeGen::set_exit_loop(unsigned LineNo) {
  exit_loop = LineNo;
}

void CodeGen::set_init_site(unsigned LineNo) {
  init_site = LineNo;
}

void CodeGen::set_finish_site(unsigned LineNo) {
  finish_site = LineNo;
}
void CodeGen::set_create_mem_site(unsigned LineNo) {
  create_mem_site = LineNo;
}

void CodeGen::add_kernel_arg(struct var_decl var) {
  var.id = kernel_args;
  arg_decls.push_back(var);
  kernel_args ++;
  //Check the var is value argument or pointer argument.
  if (var.type & 1)
    pointer_num ++;
  else
    val_num ++;

  //Check the var is independ to streams and tasks or not.
  if ((var.type & 2) == 0)
    fix_decls.push_back(var);
}

void CodeGen::add_local_var(struct var_decl var) {
  var_decls.push_back(var);
}

void CodeGen::set_replace_line(unsigned n) {
  replace_line = n;
}

void CodeGen::set_logical_streams(unsigned n) {
  logical_streams = n;
}

void CodeGen::set_task_blocks(unsigned n) {
  task_blocks = n;
}

void CodeGen::set_include_site (unsigned n){
  include_insert_site = n;
}
void CodeGen::set_cuda_site (unsigned n){
  cuda_kernel_site = n;
}

void CodeGen::set_length_var(std::string name) {
  length_var_name = name;
}
void CodeGen::set_loop_var(std::string name) {
  loop_var = name;
}
void CodeGen::set_start_index(std::string name) {
  start_index_str = name;
}

void CodeGen::add_mem_xfer(struct mem_xfer m) {
  mem_bufs.push_back (m);

  if (m.type & 1) 
    pre_xfers.push_back (m);
  else if (m.type & 2) 
    h2d_xfers.push_back (m);

  if (m.type & 8)
    post_xfers.push_back(m);
  else if (m.type & 4) 
    d2h_xfers.push_back (m);
}
void CodeGen::generateOCLDevFile() {
  if (enter_loop <= 0)
  return;

fstream Infile(InputFile.c_str());
if (!Infile){
  cerr << "\nError. File " << InputFile << " has not found.\n";
  return;
}
std::string Line = std::string();
ofstream File("kernel.cl");
cerr << "\nWriting output to dev file " << DevFile<< "\n";

unsigned LineNo = 1;
while (!Infile.eof()) {
  Line = std::string();
  std::getline(Infile, Line);

  if (replace_line == LineNo) {
    std::string::size_type start_i = Line.find("for");
    if (start_i != std::string::npos) {
      std::string::size_type init_begin, init_end, cond_begin, cond_end;
      init_begin = Line.find("=", start_i) + 1;
      init_end = Line.find(";", init_begin);
      cond_begin = Line.find("<", init_end) + 1;
      cond_end = Line.find(";", cond_begin);

      Line.replace(cond_begin, cond_end - cond_begin, " end_index");
      Line.replace(init_begin, init_end - init_begin, " start_index");
      loop_var = Line.substr(init_end + 1, cond_begin - init_end - 2);
    }

    File << "\n";
  }
  else if (is_include (Line) || in_loop(LineNo)) {
    File << Line << "\n";
  }
  if (enter_loop == LineNo) {
    //Add arguments decl.
    std::string parameters_str;
    for (unsigned i = 0; i < arg_decls.size(); i++) {
      if ( i > 0)
	parameters_str += ",\n\t";
      std::string decl_str = arg_decls[i].type_name;
      std::string::size_type idx = decl_str.find("*");
      if (idx == std::string::npos) {
        decl_str += " ";
	decl_str += arg_decls[i].var_name;
      }
      else {
        //Handle for mult-dim array, its decl should be TYPE (*name)[nn]
	decl_str.insert(idx+1, arg_decls[i].var_name);
	parameters_str += "__global ";
      }
      std::string::size_type t_idx = decl_str.find("const");
      if (t_idx != std::string::npos)
        decl_str.erase (t_idx, 5);
      parameters_str += decl_str;
    }
    

    //Add kernel function decl.
    File << "__kernel void my_kernel\n"
	 <<" ( "<<parameters_str;
    File <<")\n{\n\n";

    //Add local variables decl.
    int is_def = 0;
    for (unsigned i = 0; i < var_decls.size(); i++) {
      File <<"  "<<var_decls[i].type_name
	<<" "<<var_decls[i].var_name<<";\n";
      if (var_decls[i].var_name == loop_var)
	is_def = 1;
    }

    if (is_def == 0)
      File << "int "<<loop_var<<" = get_global_id(0);\n";
    else
      File <<loop_var<<" = get_global_id(0);\n";

    File <<"\n";
  }
  if (exit_loop == LineNo) {
    File << Line <<"\n}";
  }

  LineNo++;
}

File.close();
}

void CodeGen::generateOCLHostFile() {
  if (enter_loop <= 0)
  return;

  fstream Infile(InputFile.c_str());
  if (!Infile){
    cerr << "\nError. File " << InputFile << " has not found.\n";
    return;
  }
  std::string Line = std::string();
  ofstream File(HostFile.c_str());
  cerr << "\nWriting output to host file " << HostFile<< "\n";

  //Add hStreams include file;
  File << "#include <set_env.h>\n";

  unsigned LineNo = 1;
  while (!Infile.eof()) {
    Line = std::string();
    std::getline(Infile, Line);

  if (init_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams init code
    File <<Start<<"read_cl_file();\n"
         <<Start<<"cl_initialization();\n"
	 <<Start<<"cl_load_prog();\n\n";
  }
  if (create_mem_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams buf create code
    for (unsigned i = 0; i < mem_bufs.size(); i++) {
      File <<Start<<"cl_mem "<<mem_bufs[i].buf_name<<"_mem_obj = "
	   <<"clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, "
	   <<mem_bufs[i].size_string<<", NULL, NULL);\n";
    }
  }
  if (enter_loop == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add OpenCL mem transfer code
    for (unsigned i = 0;i < pre_xfers.size(); i++) {
      File <<Start<<"errcode = clEnqueueWriteBuffer(clCommandQue[0], "
	   <<pre_xfers[i].buf_name<<"_mem_obj, CL_TRUE, 0,\n"
	   <<Start<<pre_xfers[i].size_string<<", \n"
	   <<Start<<pre_xfers[i].buf_name<<", 0, NULL, NULL);\n";
    }

    //Add hStreams multStreams variables decl
    File <<Start<<"size_t localThreads[1] = {8};\n";

    //Add hStreams kernel arguments set code
    for (unsigned i = 0; i < fix_decls.size(); i++) {
      std::string::size_type idx = fix_decls[i].type_name.find("*");

      File <<Start<<"clSetKernelArg(clKernel, "
  	   <<fix_decls[i].id<<", ";
      if (idx != std::string::npos)
        File <<"sizeof(cl_mem), (void *) &"<<fix_decls[i].var_name<<"_mem_obj);\n";
      else
        File <<"sizeof("<<fix_decls[i].type_name<<"), &"<<fix_decls[i].var_name<<");\n";
    }

    //Use idx_subtask is to avoid be the same to original variable name
    File <<Start<<"for (int i = 0; i < tasks; i++)\n"
	 <<Start<<"{\n"
	 <<Start<<"  size_t globalOffset[1] = {i*"<<length_var_name<<"/tasks+"<<start_index_str<<"};\n"
	 <<Start<<"  size_t globalThreads[1] = {"<<length_var_name<<"/tasks"<<"};\n";

    for (unsigned i = 0;i < h2d_xfers.size(); i++) {
      File <<Start<<"  clEnqueueWriteBuffer(clCommandQue[i], "
	   <<h2d_xfers[i].buf_name<<"_mem_obj, CL_FALSE, i*"
	   <<h2d_xfers[i].size_string<<"/tasks, "
	   <<h2d_xfers[i].size_string<<"/tasks, "
	   <<"&"<<h2d_xfers[i].buf_name<<"[i*"<<length_var_name<<"/tasks]";
      for (unsigned j = 1; j < h2d_xfers[i].dim; j++)
	File <<"[0]";
      File <<", 0, NULL, NULL);\n";
    }

    File <<Start<<"  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);\n";

    for (unsigned i = 0;i < d2h_xfers.size(); i++) {
      File <<Start<<"  clEnqueueReadBuffer(clCommandQue[i], "
	   <<d2h_xfers[i].buf_name<<"_mem_obj, CL_FALSE, i*"
	   <<d2h_xfers[i].size_string<<"/tasks, "
	   <<d2h_xfers[i].size_string<<"/tasks, "
	   <<"&"<<d2h_xfers[i].buf_name<<"[i*"<<length_var_name<<"/tasks]";
      for (unsigned j = 1; j < d2h_xfers[i].dim; j++)
	File <<"[0]";
      File <<", 0, NULL, NULL);\n";
    }
    File <<Start<<"}\n";

    File <<Start<<"for (int i = 0; i < tasks; i++)\n"
    	 <<Start<<"  clFinish(clCommandQue[i]);\n";
    for (auto &mem_xfer : post_xfers) {
      File <<Start<<"errcode = clEnqueueReadBuffer(clCommandQue[0], "
	   <<mem_xfer.buf_name<<"_mem_obj, CL_TRUE, 0,\n"
	   <<Start<<mem_xfer.size_string<<", \n"
	   <<Start<<mem_xfer.buf_name<<", 0, NULL, NULL);\n";
    }
    if (post_xfers.size() > 0)
      File<<Start<<"clFinish(clCommandQue[0]);\n";

    LineNo++;
    continue;
  }
  if (in_loop(LineNo)) {
    LineNo++;
    continue;
  }
  if (finish_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }

    for (unsigned i = 0; i < mem_bufs.size(); i++)
      File <<Start<<"clReleaseMemObject("<<mem_bufs[i].buf_name<<"_mem_obj);\n";
    File<<Start<<"cl_clean_up();\n";
  }
  if (exit_loop == LineNo) {
    LineNo++;
    continue;
  }

  File << Line <<"\n";
  LineNo++;
  }

  File.close();
}

std::string CodeGen::generateCUKernelCode() {

  ostringstream File("");
  if (enter_loop <= 0)
  return File.str();

fstream Infile(InputFile.c_str());
if (!Infile){
  cerr << "\nError. File " << InputFile << " has not found.\n";
  return File.str();
}
std::string Line = std::string();

unsigned LineNo = 1;
while (!Infile.eof()) {
  Line = std::string();
  std::getline(Infile, Line);

  if (replace_line == LineNo) {
    std::string::size_type start_i = Line.find("for");
    if (start_i != std::string::npos) {
      std::string::size_type init_begin, init_end, cond_begin, cond_end;
      init_begin = Line.find("=", start_i) + 1;
      init_end = Line.find(";", init_begin);
      cond_begin = Line.find("<", init_end) + 1;
      cond_end = Line.find(";", cond_begin);

      Line.replace(cond_begin, cond_end - cond_begin, " end_index");
      Line.replace(init_begin, init_end - init_begin, " start_index");
      loop_var = Line.substr(init_end + 1, cond_begin - init_end - 2);
    }
    File << "\n";
  }
  else if (in_loop(LineNo)) {
    File << Line << "\n";
  }
  if (enter_loop == LineNo) {
    //Add arguments decl.
    std::string parameters_str;
    for (unsigned i = 0; i < fix_decls.size(); i++) {
      if ( i > 0)
	parameters_str += ", ";
      std::string decl_str = fix_decls[i].type_name;
      decl_str += " ";
      decl_str += fix_decls[i].var_name;
      parameters_str += decl_str;
    }
    parameters_str += ", int length";

    //Add kernel function decl.
    File << "__global__ void my_kernel\n"
	 <<" ( "<<parameters_str;
    File <<")\n{\n\n";

    //Add local variables decl.
    int is_def = 0;
    for (unsigned i = 0; i < var_decls.size(); i++) {
      File <<"  "<<var_decls[i].type_name
	<<" "<<var_decls[i].var_name<<";\n";
      if (var_decls[i].var_name == loop_var)
	is_def = 1;
    }

    if (is_def == 0)
      File << "int "<<loop_var<<" = blockDim.x * blockIdx.x + threadIdx.x;\n";
    else
      File <<loop_var<<" = blockDim.x * blockIdx.x + threadIdx.x;\n";

    File <<"if ("<<loop_var<<" >= length"<<")\n"
	 << "  return;\n";
  }
  if (exit_loop == LineNo) {
    File << Line <<"\n}\n\n";
  }

  LineNo++;
}

return File.str();
}

void CodeGen::generateCUDAFile() {
  if (enter_loop <= 0)
  return;

  std::string kernel_code = generateCUKernelCode();

  fstream Infile(InputFile.c_str());
  if (!Infile){
    cerr << "\nError. File " << InputFile << " has not found.\n";
    return;
  }
  std::string Line = std::string();
  std::string::size_type idx = HostFile.find(".");
  if (idx != std::string::npos)
    HostFile.erase(idx, 5);
  HostFile += ".cu";
  ofstream File(HostFile.c_str());
  cerr << "\nWriting output to CUDA file " << HostFile<< "\n";

  unsigned LineNo = 1;
  while (!Infile.eof()) {
    Line = std::string();
    std::getline(Infile, Line);

  //Add CUDA include file;
  if (LineNo == include_insert_site) {
    File << "#include <cuda.h>\n";
    File << "#include <cuda_runtime_api.h>\n\n\n";
  }
  if (LineNo == cuda_kernel_site) {
    File << kernel_code;
  }

  if (init_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams init code
    File <<Start<<"int nstreams = 2;\n"
         <<Start<<"cudaSetDevice(0);\n"
         <<Start<<"cudaSetDeviceFlags(cudaDeviceBlockingSync);\n"
	 <<Start<<"cudaStream_t *streams = (cudaStream_t*) malloc(nstreams*sizeof(cudaStream_t));\n"
         <<Start<<"for (int i = 0; i < nstreams; i++) {\n"
         <<Start<<"  cudaStreamCreate(&(streams[i]));\n"
         <<Start<<"}\n\n"
	 <<Start<<"cudaEvent_t start_event, stop_event;\n"
	 <<Start<<"int eventflags = cudaEventBlockingSync;\n"
	 <<Start<<"cudaEventCreateWithFlags(&start_event, eventflags);\n"
	 <<Start<<"cudaEventCreateWithFlags(&stop_event, eventflags);\n";
  }
  if (create_mem_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams buf create code
    for (unsigned i = 0; i < mem_bufs.size(); i++) {
      File <<Start<<mem_bufs[i].type_name<<" d_"<<mem_bufs[i].buf_name<<";\n"
	   <<Start<<"cudaMalloc((void **)&d_"<<mem_bufs[i].buf_name<<", "
	   <<mem_bufs[i].size_string<<");\n";
    }
  }
  if (enter_loop == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add OpenCL mem transfer code
    for (unsigned i = 0;i < pre_xfers.size(); i++) {
      File <<Start<<"cudaMemcpyAsync(d_"
	   <<pre_xfers[i].buf_name<<", "<<pre_xfers[i].buf_name<<", "
	   <<Start<<pre_xfers[i].size_string<<", cudaMemcpyHostToDevice, streams[0]);\n";
    }

    //Add hStreams multStreams variables decl
    File <<Start<<"int threadsPerBlock = 256;\n";

    //Add hStreams kernel arguments set code
    ostringstream parameters_os;
    for (unsigned i = 0; i < fix_decls.size(); i++) {
      if ( i > 0)
	parameters_os << ", ";

      std::string::size_type idx = fix_decls[i].type_name.find("*");

      if (idx != std::string::npos)
        parameters_os << "d_"<<fix_decls[i].var_name<<"+i*"<<length_var_name<<"/nstreams";
      else
        parameters_os <<fix_decls[i].var_name;
    }
    parameters_os << ", "<<length_var_name<<"/nstreams";

    //Use idx_subtask is to avoid be the same to original variable name
    File <<Start<<"for (int i = 0; i < nstreams; i++)\n"
	 <<Start<<"{\n"
	 <<Start<<"int blocksPerGrid = ("<<length_var_name<<"+threadsPerBlock - 1)/(nstreams*threadsPerBlock);\n";

    for (unsigned i = 0;i < h2d_xfers.size(); i++) {
      File <<Start<<"  cudaMemcpyAsync(d_"
	   <<h2d_xfers[i].buf_name<<"+i*"<<length_var_name<<"/nstreams, "
	   <<h2d_xfers[i].buf_name<<"+i*"<<length_var_name<<"/nstreams, "
	   <<h2d_xfers[i].size_string<<"/nstreams, cudaMemcpyHostToDevice, streams[i]);\n";
    }

    File <<Start<<"  my_kernel<<<blocksPerGrid, threadsPerBlock,0, streams[i]>>>("<<parameters_os.str()<<");\n";

    for (unsigned i = 0;i < d2h_xfers.size(); i++) {
      File <<Start<<"  cudaMemcpyAsync("
	   <<d2h_xfers[i].buf_name<<"+i*"<<length_var_name<<"/nstreams, "
	   <<"d_"<<d2h_xfers[i].buf_name<<"+i*"<<length_var_name<<"/nstreams, "
	   <<d2h_xfers[i].size_string<<"/nstreams, cudaMemcpyDeviceToHost, streams[i]);\n";
    }
    File <<Start<<"}\n";

    File <<Start<<"cudaEventRecord(stop_event, 0);\ncudaEventSynchronize(stop_event);\n";

    for (auto &mem_xfer : post_xfers) {
      File <<Start<<"cudaMemcpyAsync("
	   <<mem_xfer.buf_name<<", d_"
	   <<mem_xfer.buf_name<<", "
	   <<mem_xfer.size_string<<",cudaMemcpyDeviceToHost, streams[0];\n";
    }
    if (post_xfers.size() > 0)
      File <<Start<<"cudaEventRecord(stop_event, 0);\ncudaEventSynchronize(stop_event);\n";

    LineNo++;
    continue;
  }
  if (in_loop(LineNo)) {
    LineNo++;
    continue;
  }
  if (finish_site == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    File<<Start<<"for (int i = 0; i < nstreams; i++) {\n"
	<<Start<<"  cudaStreamDestroy(streams[i]);\n"
	<<Start<<"}\n"
	<<Start<<"cudaEventDestroy(start_event);\n"
	<<Start<<"cudaEventDestroy(stop_event);\n";
    for (unsigned i = 0; i < mem_bufs.size(); i++)
      File <<Start<<"cudaFree(d_"<<mem_bufs[i].buf_name<<");\n";
    File<<Start<<"cudaDeviceReset();\n";
  }
  if (exit_loop == LineNo) {
    LineNo++;
    continue;
  }

  File << Line <<"\n";
  LineNo++;
  }

  File.close();
}
//===-------------------------- CodeGen.cpp --------------------------===//

