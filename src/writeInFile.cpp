//===------------------------ writeInFile.cpp --------------------------===//
//
//
// This file is distributed under the Universidade Federal de Minas Gerais -
// UFMG Open Source License. See LICENSE.TXT for details.
//
// Copyright (C) 2018  Peng Zhang 
//
//===----------------------------------------------------------------------===//
//
// 
//===----------------------------------------------------------------------===//
#include <fstream>
#include <iostream>


#include "writeInFile.h" 

#define CarriageReturn 13

using namespace std;

#define DEBUG_TYPE "writeInFile"


void WriteInFile::generateDevFile() {
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

      std::string::size_type t_idx = decl_str.find("double");
      if (t_idx == std::string::npos)
        t_idx = decl_str.find("float");
      //Handle double/float scalar parameters, its conversion expression
      //should be like this: TYPE a = *((TYPE *) (&arg));
      if (idx == std::string::npos && t_idx != std::string::npos) {
        File <<"  "<<decl_str
	  <<" = *((" <<arg_decls[i].type_name
	  <<" *) (&arg" <<arg_decls[i].id <<"));\n";
      }
      else {
        File <<"  "<<decl_str
	  <<" = (" <<arg_decls[i].type_name
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

void WriteInFile::generateHostFile() {
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

  if (init_cite == LineNo) {
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

    //Add hStreams buf create code
    for (unsigned i = 0; i < mem_bufs.size(); i++) {
      File <<Start<<"(hStreams_app_create_buf("
	   <<"("<<mem_bufs[i].type_name<<")"
	<<mem_bufs[i].buf_name<<", "<<mem_bufs[i].size_string<<"));\n";
    }

    //Add hStreams mem transfer code
    for (unsigned i = 0;i < pre_xfers.size(); i++) {
      File <<Start<<"(hStreams_app_xfer_memory("
	   <<"("<<mem_bufs[i].type_name<<")"
	   <<pre_xfers[i].buf_name<<", "
	   <<"("<<mem_bufs[i].type_name<<")"
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
    //Init start_index.
    File <<Start<<"start_index = 0;\n";

    File <<Start<<"for (int i = 0; i < "<<task_blocks<<"; i++)\n"
	 <<Start<<"{\n"
	 <<Start<<"  args[0] = (uint64_t) start_index;\n"
	 <<Start<<"  end_index = start_index + sub_blocks;\n"
	 <<Start<<"  if (i < remain_index)\n"
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
	   <<"i \% "<<logical_streams<<", HSTR_SRC_TO_SINK, NULL));\n";
    }

    File <<Start<<"  (hStreams_EnqueueCompute(\n"
	 <<Start<<"			i % "<<logical_streams<<",\n"
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
	   <<"i \% "<<logical_streams<<", HSTR_SINK_TO_SRC, NULL));\n";
    }

    File <<Start<<"  start_index = end_index;\n"
         <<Start<<"}\n";

    LineNo++;
    continue;
  }
  if (in_loop(LineNo)) {
    LineNo++;
    continue;
  }
  if (finish_cite == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }

    File<<Start<<"hStreams_ThreadSynchronize();\n";
    for (auto &mem_xfer : post_xfers) {
      File <<Start<<"  (hStreams_app_xfer_memory("
	   <<"("<<mem_xfer.type_name<<"*)"
	   <<mem_xfer.buf_name<<", "
	   <<"("<<mem_xfer.type_name<<"*)"
	   <<mem_xfer.buf_name<<" ,"
	   <<mem_xfer.size_string<<", 0, HSTR_SINK_TO_SRC, NULL));\n";
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

WriteInFile::WriteInFile() {
  kernel_args = 0;
  val_num = 0;
  pointer_num = 0;
  logical_streams = 0;
  task_blocks = 0;
  replace_line = 0;
  enter_loop = 0;
  exit_loop = 0;
  init_cite = 0;
  finish_cite = 0;
}

WriteInFile::~WriteInFile() {
}

void WriteInFile::set_InputFile(std::string inputFile) {
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

bool WriteInFile::is_include (std::string line) {
  std::size_t found1 = line.find_first_not_of(" ");
  std::size_t found2 = line.find_first_not_of("\t");
  std::size_t found = found1 > found2 ? found1 : found2;
  if (found < line.size())
    return (line.compare(found, 8, "#include") == 0);
  else
    return false;
}

bool WriteInFile::in_loop (unsigned LineNo) {
  return (LineNo > enter_loop && LineNo < exit_loop);
}

void WriteInFile::set_enter_loop(unsigned LineNo) {
  enter_loop = LineNo;
}

void WriteInFile::set_exit_loop(unsigned LineNo) {
  exit_loop = LineNo;
}

void WriteInFile::set_init_cite(unsigned LineNo) {
  init_cite = LineNo;
}

void WriteInFile::set_finish_cite(unsigned LineNo) {
  finish_cite = LineNo;
}

void WriteInFile::add_kernel_arg(struct var_decl var) {
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

void WriteInFile::add_local_var(struct var_decl var) {
  var_decls.push_back(var);
}

void WriteInFile::set_replace_line(unsigned n) {
  replace_line = n;
}

void WriteInFile::set_logical_streams(unsigned n) {
  logical_streams = n;
}

void WriteInFile::set_task_blocks(unsigned n) {
  task_blocks = n;
}

void WriteInFile::set_length_var(std::string name) {
  length_var_name = name;
}

void WriteInFile::add_mem_xfer(struct mem_xfer m) {
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

//===-------------------------- writeInFile.cpp --------------------------===//

