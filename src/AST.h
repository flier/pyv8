#pragma once

#undef COMPILER
#include "src/v8.h"
#include "src/ast.h"

#include "Wrapper.h"

namespace v8i = v8::internal;

class CAstNode
{  
protected:
  v8i::AstNode *m_node;

  CAstNode(v8i::AstNode *node) : m_node(node) {}

  template <typename T>
  T *as(void) const { return static_cast<T *>(m_node); }
public:
  virtual ~CAstNode() {}

  static void Expose(void);
};

class CAstVisitor;

class CAstStatement : public CAstNode
{
protected:
  CAstStatement(v8i::Statement *stat) : CAstNode(stat) {}
public:
  operator bool(void) const { return !as<v8i::Statement>()->IsEmpty(); }
  int pos(void) const { return as<v8i::Statement>()->statement_pos(); }
};

class CAstExpression : public CAstNode
{
protected:
  CAstExpression(v8i::Expression *expr) : CAstNode(expr) {}
public:
  v8i::Expression::Context context(void) const { return as<v8i::Expression>()->context(); }
};

class CAstBreakableStatement : public CAstStatement
{
protected:
  CAstBreakableStatement(v8i::BreakableStatement *stat) : CAstStatement(stat) {}
public:
  bool anonymous(void) const { return as<v8i::BreakableStatement>()->is_target_for_anonymous(); }
};

class CAstBlock : public CAstBreakableStatement
{
public:
  CAstBlock(v8i::Block *block) : CAstBreakableStatement(block) {}

  void Accept(py::object callback);
};

class CAstDeclaration : public CAstNode
{
public:
  CAstDeclaration(v8i::Declaration *decl) : CAstNode(decl) {}

  void Accept(py::object callback);
};

class CAstIterationStatement : public CAstBreakableStatement
{
public:
  CAstIterationStatement(v8i::IterationStatement *stat) : CAstBreakableStatement(stat) {}
};

class CAstDoWhileStatement : public CAstIterationStatement
{
public:
  CAstDoWhileStatement(v8i::DoWhileStatement *stat) : CAstIterationStatement(stat) {}

  void Accept(py::object callback);
};

class CAstWhileStatement : public CAstIterationStatement
{
public:
  CAstWhileStatement(v8i::WhileStatement *stat) : CAstIterationStatement(stat) {}

  void Accept(py::object callback);
};

class CAstForStatement : public CAstIterationStatement
{
public:
  CAstForStatement(v8i::ForStatement *stat) : CAstIterationStatement(stat) {}

  void Accept(py::object callback);
};

class CAstForInStatement : public CAstIterationStatement
{
public:
  CAstForInStatement(v8i::ForInStatement *stat) : CAstIterationStatement(stat) {}

  void Accept(py::object callback);
};

class CAstExpressionStatement : public CAstStatement
{
public:
  CAstExpressionStatement(v8i::ExpressionStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstContinueStatement : public CAstStatement
{
public:
  CAstContinueStatement(v8i::ContinueStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstBreakStatement : public CAstStatement
{
public:
  CAstBreakStatement(v8i::BreakStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstReturnStatement : public CAstStatement
{
public:
  CAstReturnStatement(v8i::ReturnStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstWithEnterStatement : public CAstStatement
{
public:
  CAstWithEnterStatement(v8i::WithEnterStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstWithExitStatement : public CAstStatement
{
public:
  CAstWithExitStatement(v8i::WithExitStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstCaseClause 
{
  v8i::CaseClause *m_clause;
public:
  CAstCaseClause(v8i::CaseClause *clause) : m_clause(clause) {}
};

class CAstSwitchStatement : public CAstBreakableStatement
{
public:
  CAstSwitchStatement(v8i::SwitchStatement *stat) : CAstBreakableStatement(stat) {}

  void Accept(py::object callback);
};

class CAstIfStatement : public CAstStatement
{
public:
  CAstIfStatement(v8i::IfStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstTargetCollector : public CAstNode
{
public:
  CAstTargetCollector(v8i::TargetCollector *collector) : CAstNode(collector) {}
};

class CAstTryStatement : public CAstStatement
{
protected:
  CAstTryStatement(v8i::TryStatement *stat) : CAstStatement(stat) {}
public:
};

class CAstTryCatchStatement : public CAstTryStatement
{
public:
  CAstTryCatchStatement(v8i::TryCatchStatement *stat) : CAstTryStatement(stat) {}

  void Accept(py::object callback);
};

class CAstTryFinallyStatement : public CAstTryStatement
{
public:
  CAstTryFinallyStatement(v8i::TryFinallyStatement *stat) : CAstTryStatement(stat) {}

  void Accept(py::object callback);
};

class CAstDebuggerStatement : public CAstStatement
{
public:
  CAstDebuggerStatement(v8i::DebuggerStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstEmptyStatement : public CAstStatement
{
public:
  CAstEmptyStatement(v8i::EmptyStatement *stat) : CAstStatement(stat) {}

  void Accept(py::object callback);
};

class CAstLiteral : public CAstExpression
{
public:
  CAstLiteral(v8i::Literal *lit) : CAstExpression(lit) {}

  void Accept(py::object callback);
};

class CAstMaterializedLiteral : public CAstExpression
{
protected:
  CAstMaterializedLiteral(v8i::MaterializedLiteral *lit) : CAstExpression(lit) {}
};

class CAstObjectLiteral : public CAstMaterializedLiteral
{
public:
  CAstObjectLiteral(v8i::ObjectLiteral *lit) : CAstMaterializedLiteral(lit) {}

  void Accept(py::object callback);
};

class CAstRegExpLiteral : public CAstMaterializedLiteral
{
public:
  CAstRegExpLiteral(v8i::RegExpLiteral *lit) : CAstMaterializedLiteral(lit) {}

  void Accept(py::object callback);
};

class CAstArrayLiteral : public CAstMaterializedLiteral
{
public:
  CAstArrayLiteral(v8i::ArrayLiteral *lit) : CAstMaterializedLiteral(lit) {}

  void Accept(py::object callback);
};

class CAstCatchExtensionObject : public CAstExpression
{
public:
  CAstCatchExtensionObject(v8i::CatchExtensionObject *obj) : CAstExpression(obj) {}

  void Accept(py::object callback);
};

class CAstVariableProxy : public CAstExpression
{
public:
  CAstVariableProxy(v8i::VariableProxy *proxy) : CAstExpression(proxy) {}

  void Accept(py::object callback);
};

class CAstSlot : public CAstExpression
{
public:
  CAstSlot(v8i::Slot *slot) : CAstExpression(slot) {}

  void Accept(py::object callback);
};

class CAstProperty : public CAstExpression
{
public:
  CAstProperty(v8i::Property *prop) : CAstExpression(prop) {}

  void Accept(py::object callback);
};

class CAstCall : public CAstExpression
{
public:
  CAstCall(v8i::Call *call) : CAstExpression(call) {}

  void Accept(py::object callback);
};

class CAstCallNew : public CAstExpression
{
public:
  CAstCallNew(v8i::CallNew *call) : CAstExpression(call) {}

  void Accept(py::object callback);
};

class CAstCallRuntime : public CAstExpression
{
public:
  CAstCallRuntime(v8i::CallRuntime *call) : CAstExpression(call) {}

  void Accept(py::object callback);
};

class CAstUnaryOperation : public CAstExpression
{
public:
  CAstUnaryOperation(v8i::UnaryOperation *op) : CAstExpression(op) {}

  void Accept(py::object callback);
};

class CAstBinaryOperation : public CAstExpression
{
public:
  CAstBinaryOperation(v8i::BinaryOperation *op) : CAstExpression(op) {}

  void Accept(py::object callback);
};

class CAstCountOperation : public CAstExpression
{
public:
  CAstCountOperation(v8i::CountOperation *op) : CAstExpression(op) {}

  void Accept(py::object callback);
};

class CAstCompareOperation : public CAstExpression
{
public:
  CAstCompareOperation(v8i::CompareOperation *op) : CAstExpression(op) {}

  void Accept(py::object callback);
};

class CAstConditional : public CAstExpression
{
public:
  CAstConditional(v8i::Conditional *cond) : CAstExpression(cond) {}

  void Accept(py::object callback);
};

class CAstAssignment : public CAstExpression
{
public:
  CAstAssignment(v8i::Assignment *assignment) : CAstExpression(assignment) {}

  void Accept(py::object callback);
};

class CAstThrow : public CAstExpression
{
public:
  CAstThrow(v8i::Throw *th) : CAstExpression(th) {}

  void Accept(py::object callback);
};

class CAstFunctionLiteral : public CAstExpression
{
public:
  CAstFunctionLiteral(v8i::FunctionLiteral *func) : CAstExpression(func) {}

  void Accept(py::object callback);
};

class CAstFunctionBoilerplateLiteral : public CAstExpression
{
public:
  CAstFunctionBoilerplateLiteral(v8i::FunctionBoilerplateLiteral *func) : CAstExpression(func) {}

  void Accept(py::object callback);
};

class CAstThisFunction : public CAstExpression
{
public:
  CAstThisFunction(v8i::ThisFunction *func) : CAstExpression(func) {}

  void Accept(py::object callback);
};

class CAstVisitor : public v8i::AstVisitor
{
  py::object m_visitor;
public:
  CAstVisitor(py::object visitor) : m_visitor(visitor) {}

#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { CAst##type obj(node); m_visitor(obj); }
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT
};
