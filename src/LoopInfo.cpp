//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "LoopInfo.h"



//Make sure s->type == 1, this function create root loop for function.
LoopInfo::LoopInfo (ScopeIR *s) {
  scope = s;
  hasCollected=false;parallelizable=0;perfect=0;canonical=0;
  s->setLoopInfo(this);
}

void LoopInfo::buildLoopTree () {
  if (scope == NULL)
    return;

  children = createInnerLoop(scope);

  for (auto & child : children) {
    LoopInfo * innerloop = child;
    innerloop->buildLoopTree();
  }
}

std::vector<LoopInfo *> LoopInfo::createInnerLoop(ScopeIR *s) {

  std::vector<LoopInfo *> loopVec;
  //Traverse the scope tree, untill find one or more loops.
  for (auto & child : s->children) {
    switch (child->type) {
      case 2:
      case 3:
      case 4:
        {
	  LoopInfo * loop = new LoopInfo(child);
	  loopVec.push_back(loop);
	}
	break;
      default:
	{
	  std::vector<LoopInfo *> vec = createInnerLoop(child);
	  for (auto & v: vec)
	    loopVec.push_back(v);
	}
    }
  }

  return loopVec;
}

  // For these flags, 0 means undefine, -1 means false, 1 means true;
bool LoopInfo::isPerfect() {
  if (perfect == 1)
    return true;
  else if (perfect == -1)
    return false;

  perfect = 1;
  for (auto & child : children) {
    if (!child->isPerfect()) {
      perfect = -1;
      return false;
    }
  }

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<ScopeIR *> scopeQue;
  scopeQue.push_back(scope);
  while (perfect == 1 && scopeQue.empty() == false) {
    ScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();
    for (auto& child : scope->children) {
      switch (child->type) {
	case 2:
	case 3:
	case 4:
	  break;
	case 5:
	case 6:
	case 7:
	case 15:
	case 16:
	  perfect = -1;
	  break;
	default:
	  scopeQue.push_back(child);
      }
    }
  }

  if (perfect == 1)
    return true;

  return false;
}

bool LoopInfo::isParallelizable() {

  //TODO: We can only parallel canonical loop right now.
  if (!isCanonical()) {
    parallelizable = -1;
    return false;
  }

  //We omit the loop which has call statement.
  if (scope->get_call_num() > 0) {
    parallelizable = -1;
    return false;
  }

  if (parallelizable == 1)
    return true;
  else if (parallelizable == -1)
    return false;

  parallelizable= 1;

  //If the index of array is not loop inductive or is a scalar, then var only be read or has been initialized in loop, otherwise will have WAW/RAW/WAR dependence.
  //Else is var is loop inductive, which means var is memory access and with an index.
    //If var has only one index, there are no dependence between loop iterations.
    //Else if var has two or more indices, then var only be read or we are sure these indices are always not equal in the range of loop.
      //TODO:Right now we do not consider the second condition.
  //Collect all var accessed in this loop.
  collectVars();
  for (auto& loop_var : loopVars) {
    //Except the loop iterator.
    if (loop_var->var_decl == iter)
      continue;
    //Except the scalar variable which has been initialized in loop.
    if(loop_var->isInitialized() == true)
      continue;

    std::vector <Expr *> index = loop_var->accessInst[0]->index;//For array or memory pointer, array may has more than one dimension indices.
    int num_index = 1;
    bool has_write = false;
    int iter_related = 0;
    for (auto& av : loop_var->accessInst) {
      if (index.size() != av->index.size())
	num_index ++;
      else {
        for (unsigned int k = 0; k < av->index.size(); k ++) {
          if (av->scope->compareAlgebraExpr(index[k], av->index[k]) == false) {
            num_index ++;
	    break;
	  }
        }
      }

      if (av->isWrite()) 
        has_write = true;

      for (unsigned int k = 0; k < av->index.size(); k ++) {
        if (av->scope->isVarRelatedExpr(av->index[k], iter->decl_stmt)) {
	  iter_related ++;
	  break;
        }
      }
    }

    if (iter_related > 0 && has_write && num_index > 1)
        parallelizable = -1;
    else if (iter_related == 0 && has_write)
	parallelizable = -1;

    if (parallelizable != 1)
      break;
  }

  if (parallelizable == 1)
    return true;

  return false;
}

bool LoopInfo::isCanonical() {
  if (canonical == 1)
    return true;
  else if (canonical == -1)
    return false;

  canonical = -1;
  if (scope->type == 2) {
    if (ForStmt * f = dyn_cast<ForStmt>(scope->condition)) {
      Expr * step = f->getInc();
      if (step == NULL)
	return false;

      if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(step)) {
        switch (BinOp->getOpcode()) {
          case BO_MulAssign:
          case BO_DivAssign:
          case BO_AddAssign:
          case BO_SubAssign:
	    {
	      Expr * e = BinOp->getLHS()->IgnoreImpCasts();
	      if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	        iter = scope->find_var(d->getDecl());
	        if (iter)
	          canonical = 1;
	      }
	    }
	    break;
	  default:
	    break;
        }
      }
      else if (UnaryOperator * UnOp = dyn_cast<UnaryOperator>(step)) {
        switch (UnOp->getOpcode()) {
          case UO_PostInc:
          case UO_PostDec:
          case UO_PreInc:
          case UO_PreDec:
	    {
              Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
	      if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	        iter = scope->find_var(d->getDecl());
	        if (iter)
	          canonical = 1;
	      }
	    }
	    break;
	  default:
	    break;
	}
      }
    }
  }

  if (canonical == 1)
    return true;

  return false;
}

bool LoopInfo::isInductive(LoopVar *v) {

  //If is not canonical loop, we can not find which variable is loop iterator.
  //By default, we assumed all variables is inductive.
  //TODO: need add analysis of loop inductive variables.
  return true;
}

void LoopInfo::collectVars() {

  if (hasCollected)
    return;

  for (auto& child : children) {
    child->collectVars();
    for (auto& child_lv : child->loopVars) {
      bool merged = false;
      //Do not collect variables which are defined in loop body.
      if (scope->isInside(child_lv->var_decl->scope) == true)
	continue;
      LoopVar * new_lv = child_lv->copy();
      for (auto& lv : loopVars)
	merged = lv->tryMerge(new_lv);
      if (merged == false)
	loopVars.push_back(new_lv);
    }
  }

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<ScopeIR *> scopeQue;
  scopeQue.push_back(scope);
  while (scopeQue.empty() == false) {
    ScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();

    for (auto& av: scope->access_chain)
      addVar(av);

    for (auto& child : scope->children) {
      switch (child->type) {
	case 2:
	case 3:
	case 4:
	  break;
	default:
	  scopeQue.push_back(child);
      }
    }
  }

  hasCollected = true;
}

void LoopInfo::addVar(Access_Var *av) {
  for (auto& lv : loopVars) {
    if (av->get_decl() == lv->var_decl) {
      lv->addAV(av);
      return;
    }
  }

  if (scope->isInside(av->get_decl()->scope) == true)
    return;

  LoopVar * lv = new LoopVar();
  lv->var_decl = av->get_decl();
  lv->addAV(av);
  loopVars.push_back(lv);
}

int LoopInfo::get_start_line(){
  return scope->get_start_pos()->get_line();
}

void LoopInfo::dump() {
  scope->dump();
}

//TODO: Need to complete and replace the old KernelExtractor.
void LoopInfo::genKernelInfo(Kernel_Info &ki){
//  unsigned enter_loop;
//  unsigned exit_loop;
//  unsigned init_site ;
//  unsigned finish_site;
//  unsigned create_mem_site;
//  unsigned replace_line;
//  std::vector<mem_xfer> mem_bufs;
//  std::vector<var_decl> val_parms;
//  std::vector<var_decl> pointer_parms;
//  std::vector<var_decl> local_parms;
//  std::string length_var;
//  std::string loop_var;
//
//  //The omp parallel for iteration index variable declaration
//  ki.loop_index = cast<ValueDecl>(iter->decl_stmt);
//  //The compute instructions.
//  unsigned int insns;
//  //The init value of loop index may not be 0!
//  std::string start_index;
}


bool LoopVar::tryMerge(LoopVar * lv) {
  if (lv->var_decl != var_decl)
    return false;

  for (auto& av : lv->accessInst)
    addAV(av);

  return true;
}

void LoopVar::addAV(Access_Var *av) {

  //Check if already exist.
  for (auto& ac_va : accessInst)
    if (av == ac_va)
      return;

  accessInst.push_back(av);
}

bool LoopVar::isInitialized() {
  Access_Var * first = accessInst.front();
  for (auto&av : accessInst) {
    if (*(first->pos) > *(av->pos))
      first = av;
  }

  if (first->isMem() == false && first->isWrite() == true)
    return true;

  return false;
}

LoopVar * LoopVar::copy() {
  LoopVar * ret = new LoopVar();
  ret->var_decl = var_decl;
  ret->accessInst = accessInst;
  return ret;
}
