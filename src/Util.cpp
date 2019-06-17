//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

class ExprSerializer: public RecursiveASTVisitor<ExprSerializer> {

public:
  void clear() {output.clear();}
  std::vector<Expr *> getOutput {return output;}

  bool VisitStmt(Stmt *s) {
    if (s == NULL)
      return true;
    switch (s->getStmtClass()) {
      case clang::Stmt::BinaryOperator:
      case clang::Stmt::CompoundAssignOperator:
      case clang::Stmt::UnaryOperator:
      case clang::Stmt::DeclRefExpr:
      case clang::Stmt::IntegerLiteral:
      case clang::Stmt::FloatingLiteral:
      case clang::Stmt::ImaginaryLiteral:
      case clang::Stmt::ParenExpr:
      case clang::Stmt::ArraySubscriptExpr:
      case clang::Stmt::MemberExpr:
	output.push_back(s);
	break;
      default:
	break;
    }

    return true;
  }

private:
  std::vector<Expr *> output;
}
