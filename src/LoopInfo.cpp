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

  for (int i = 0; i < children.size(); i++) {
    LoopInfo * innerloop = children[i];
    innerloop->buildLoopTree();
  }
}

std::vector<LoopInfo *> LoopInfo::createInnerLoop(ScoepIR *s) {

  std::vector<LoopInfo *> loopVec;
  //Traverse the scope tree, untill find one or more loops.
  for (int i = 0; i < scope->children.size(); i++) {
    ScopeIR * subScope = scope->children[i];
    switch (subScope->type) {
      case 2:
      case 3:
      case 4:
        LoopInfo * loop = new LoopInfo(subScope);
	loopVec.push_back(loop);
	break;
      default:
	std::vector<LoopInfo *> vec = createInnerLoop(subScope);
	for (int j = 0; j < vec.size(); j++)
	  loopVec.push_back(vec[j]);
	break;
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
  for (int i = 0; i < children.size(); i++) {
    if (!children[i]->isPerfect()) {
      perfect = -1;
      return false;
    }
  }

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<ScopeIR *> scopeQue;
  scopeQue.push_back(scope);
  while (perfect == 1 && scopeQue.size() > 0) {
    ScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();
    for (int i = 0; i < scope->children.size(); i++) {
      ScopeIR * child = scope->children[i];
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
	default;
	  scopeQue.push_back(child);
      }
    }
  }

  if (perfect == 1)
    return true;
  else if (perfect == -1)
    return false;
}

bool LoopInfo::isParallelizable() {

  //TODO: We can only parallel canonical loop right now.
  if (!isCanonical()) {
    parallelizable = -1;
    return false;
  }

  if (parallelizable == 1)
    return true;
  else if (parallelizable == -1)
    return false;

  parallelizable= 1;

  //If var is not loop inductive, then var only be read, otherwise will have WAW/RAW/WAR dependence.
  //Else is var is loop indective, which means var is memory access and with an index.
    //If var has only one index, there are no dependence between loop iterations.
    //Else if var has two or more indices, then var only be read or we are sure these indices are always not equal in the range of loop.
      //TODO:Right now we do not consider the second condition.
  //Collect all var accessed in this loop.
  collectVars();
  for (int i = 0; i < loopVars.size() && parallelizable==1; i++) {
    std::vector <Expr *> index = loopVars[i]->accessInst[0];//For array or memory pointer, array may has more than one dimension indices.
    int num_index = 1;
    bool has_write = false;
    int iter_related = 0;
    for (int j = 0; j < loopVars[i]->accessInst.size() && parallelizable==1; j++) {
      Access_Var * av = loopVars[i]->accessInst[j];
      if (index.size() != av->index.size())
	diff_index ++;
      else {
        for (int k = 0; k < av->index.size(); k ++) {
          if (scope->compareAlgebraExpr(index[k], av->index[k]) == false) {
            diff_index ++;
	    break;
	  }
        }
      }

      if (av->isWrite()) 
        has_write = true;

      for (int k = 0; k < av->index.size(); k ++) {
        if (scope->isVarRelatedExpr(av->index[k], iter->decl_stmt) {
	  iter_related ++;
	  break;
        }
      }
    }

    if (iter_related > 0 && has_write && num_index > 1)
        parallelizable = -1;
    else if (iter_related == 0 && has_write)
	parallelizable = -1;
  }

  if (parallelizable == 1)
    return true;
  else if (parallelizable == -1)
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
      if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(step)) {
        switch (BinOp->getOpcode() ) {
          case BO_MulAssign:
          case BO_DivAssign:
          case BO_AddAssign:
          case BO_SubAssign:
	    Expr * e= BinOp->getLHS()->IgnoreImpCasts();
	    if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	      iter = scope->find_var(d->getDecl());
	      if (iter)
	        canonical = 1;
	    }
	    break;
	  default:
	    break;
        }
      }
      else if (UnaryOperator * UnOp = dyn_cast<UnaryOperator>(step)) {
        switch (UnOp->getOpcode()) {
          case UO_PostInc:
          case UO_PostDes:
          case UO_PreInc:
          case UO_PreDec:
            Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
	    if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	      iter = scope->find_var(d->getDecl());
	      if (iter)
	        canonical = 1;
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
  else if (canonical == -1)
    return false;
}

bool LoopInfo::isInductive(LoopVar *v) {

  //If is not canonical loop, we can not find which variable is loop iterator.
  //By default, we assumed all variables is inductive.
  if (isCanonical() == false)
    return true;

  if (inductive == 1)
    return true;
  else if (inductive == -1)
    return false;
  
  inductive = -1;

//TODO: need add analysis of loop inductive variables.

  if (inductive == 1)
    return true;
  else if (inductive == -1)
    return false;
}

void LoopInfo::collectVars() {

  if (hasCollected)
    return;

  for (int i = 0; i < children.size(); i++) {
    children[i]->collectVar();
    for(int j = 0; j < children[i]->loopVars.size(); j++) {
      bool merged = false;
      for (int k = 0; k < loopVars.size() && merged == false; k++)
	merged = loopVars[k]->tryMerge(children[i]->loopVars[j]);
      if (merged == false)
	loopVars.push_back(children[i]->loopVars[j]);
    }
  }

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<ScopeIR *> scopeQue;
  scopeQue.push_back(scope);
  while (scopeQue.size() > 0) {
    ScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();

    for (int i = 0; i < scope->access_chain.size(); i++)
      addVar(scope->access_chain[i]);

    for (int i = 0; i < scope->children.size(); i++) {
      ScopeIR * child = scope->children[i];
      switch (child->type) {
	case 2:
	case 3:
	case 4:
	  break;
	default;
	  scopeQue.push_back(child);
      }
    }
  }

  hasCollected = true;
}

void LoopInfo::addVar(Access_Var *av) {
  for (int i = 0; i < loopVars.size(); i++) {
    if (av->get_decl() == loopVars[i]->var_decl) {
      loopVars[i]->addAV(av);
      return;
    }
  }

  LoopVar * lv = new LoopVar();
  lv->var_decl = av->get_decl();
  lv->addAV(av);
  loopVars.push_back(lv);
}

//TODO: Need to complete and replace the old KernelExtractor.
void LoopInfo::genKernelInfo(Kernel_Info &ki){
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
  ki.loop_index = iter->decl_stmt;
  //The compute instructions.
  unsigned int insns;
  //The init value of loop index may not be 0!
  std::string start_index;
}


bool LoopVar::tryMerge(LoopVar * lv) {
  if (lv->var_decl != var_decl)
    return false;

  for (int i = 0; i < lv->accessInst.size(); i++)
    addAV(lv->accessInst[i]);

  return true;
}

void LoopVar::addAV(Access_Var *av) {

  //Check if already exist.
  for (int i = 0; i < accessInst.size(); i++) {
    if (av == accessInst[i])
      return;
  }

  accessInst.push_back(av);
}
