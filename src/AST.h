#pragma once

#include <vector>

#ifndef WIN32

#ifndef isfinite
#define isfinite(val) (val <= std::numeric_limits<double>::max())
#endif

#endif

#undef COMPILER
#include "src/v8.h"
#include "src/ast.h"
#include "src/scopes.h"
#include "src/assembler.h"

#include "utf8.h"

#include "PrettyPrinter.h"
#include "Wrapper.h"

namespace v8i = v8::internal;

template <typename T>
inline py::object to_python(T& obj)
{
  return py::object(py::handle<>(py::to_python_value<T&>()(obj)));
}

template <typename T>
inline py::object to_python(T *node);

template <typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst);

template <typename C, typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst);

template <typename T>
T _to_string(v8i::Handle<v8i::String> str)
{ 
  if (str.is_null()) return T();

  v8i::String::FlatContent content = str->GetFlatContent();

  if (content.IsFlat())
  {
    if (content.IsAscii())
    {
      v8i::Vector<const char> buf = content.ToAsciiVector();

      return T(buf.start(), buf.length());
    }
    else
    {
      v8i::Vector<const v8i::uc16> buf = content.ToUC16Vector();
      std::vector<char> out;

      utf8::utf16to8(buf.start(), buf.start() + buf.length(), out.begin());

      return T(&out[0], out.size());
    }
  }
  else
  {
    int len = 0;
    v8i::SmartArrayPointer<char> buf = str->ToCString(v8i::DISALLOW_NULLS, v8i::FAST_STRING_TRAVERSAL, &len);

    return T(*buf, len);
  }
}

inline py::object to_python(v8i::Handle<v8i::String> str)
{
  return _to_string<py::str>(str);
}

inline const std::string to_string(v8i::Handle<v8i::String> str)
{
  return _to_string<std::string>(str);
}

class CAstVisitor;
class CAstVariable;
class CAstVariableProxy;
class CAstFunctionLiteral;

class CAstScope 
{
  v8i::Scope *m_scope;
public:
  CAstScope(v8i::Scope *scope) : m_scope(scope) {}

  bool IsEval(void) const { return m_scope->is_eval_scope(); }
  bool IsFunction(void) const { return m_scope->is_function_scope(); }
  bool IsGlobal(void) const { return m_scope->is_global_scope(); }

  bool CallsEval(void) const { return m_scope->calls_eval(); }
  bool OuterScopeCallsEval(void) const { return m_scope->calls_eval(); }

  bool InsideWith(void) const { return m_scope->inside_with(); }
  bool ContainsWith(void) const { return m_scope->contains_with(); }

  py::object GetOuter(void) const { v8i::Scope *scope = m_scope->outer_scope(); return scope ? py::object(CAstScope(scope)) : py::object(); }

  py::list GetDeclarations(void) const { return to_python(m_scope->declarations()); }

  py::object GetReceiver(void) const;
  py::object GetFunction(void) const;
  int GetParametersNumer(void) const { return m_scope->num_parameters(); }
  CAstVariable GetParameter(int index) const;
  py::object GetArguments(void) const;
};

class CAstVariable 
{
  v8i::Variable *m_var;
public:
  CAstVariable(v8i::Variable *var) : m_var(var) {}

  v8i::Variable *GetVariable(void) const { return m_var; }

  bool IsValidLeftHandSide(void) const { return m_var->IsValidLeftHandSide(); }

  CAstScope scope(void) const { return CAstScope(m_var->scope()); }
  py::object name(void) const { return to_python(m_var->name()); }
  v8i::VariableMode mode(void) const { return m_var->mode(); }

  bool is_global(void) const { return m_var->is_global(); }
  bool is_this(void) const { return m_var->is_this(); }
  bool is_arguments(void) const { return m_var->is_arguments(); }
  bool is_possibly_eval(void) const { return m_var->is_possibly_eval(); }
};

class CAstLabel
{
  v8i::Label *m_label;
public:
  CAstLabel(v8i::Label *label) : m_label(label)
  {
  }

  int GetPosition(void) const { return m_label->pos(); }
  bool IsBound(void) const { return m_label->is_bound(); }
  bool IsUnused(void) const { return m_label->is_unused(); }
  bool IsLinked(void) const { return m_label->is_linked(); }
};

class CAstNode
{  
protected:
  v8i::AstNode *m_node;

  CAstNode(v8i::AstNode *node) : m_node(node) {}

  void Visit(py::object handler);
public:
  virtual ~CAstNode() {}

  template <typename T>
  T *as(void) const { return static_cast<T *>(m_node); }

  static void Expose(void);

  v8i::AstNode::Type GetType(void) const { return m_node->node_type(); }

  const std::string ToString(void) const { return v8i::PrettyPrinter().Print(m_node); }
};

class CAstStatement : public CAstNode
{
protected:
  CAstStatement(v8i::Statement *stat) : CAstNode(stat) {}
public:
  operator bool(void) const { return !as<v8i::Statement>()->IsEmpty(); }

  int GetPosition(void) const { return as<v8i::Statement>()->statement_pos(); }
  void SetPosition(int pos) { as<v8i::Statement>()->set_statement_pos(pos); }
};

class CAstExpression : public CAstNode
{
protected:
  CAstExpression(v8i::Expression *expr) : CAstNode(expr) {}
public:
  bool IsPropertyName(void) { return as<v8i::Expression>()->IsPropertyName(); }
};

class CAstBreakableStatement : public CAstStatement
{
protected:
  CAstBreakableStatement(v8i::BreakableStatement *stat) : CAstStatement(stat) {}
public:
  bool IsTargetForAnonymous(void) const { return as<v8i::BreakableStatement>()->is_target_for_anonymous(); }  

  CAstLabel GetBreakTarget(void) { return CAstLabel(as<v8i::BreakableStatement>()->break_target()); }
};

class CAstBlock : public CAstBreakableStatement
{
public:
  CAstBlock(v8i::Block *block) : CAstBreakableStatement(block) {}

  void AddStatement(CAstStatement& stmt) { as<v8i::Block>()->AddStatement(stmt.as<v8i::Statement>()); }

  py::list GetStatements(void) { return to_python(as<v8i::Block>()->statements()); }

  bool IsInitializerBlock(void) const { return as<v8i::Block>()->is_initializer_block(); }
};

class CAstDeclaration : public CAstNode
{
public:
  CAstDeclaration(v8i::Declaration *decl) : CAstNode(decl) {}

  CAstVariableProxy GetProxy(void) const;
  v8i::VariableMode GetMode(void) const { return as<v8i::Declaration>()->mode(); }
  py::object GetFunction(void) const;
};

class CAstIterationStatement : public CAstBreakableStatement
{
protected:
  CAstIterationStatement(v8i::IterationStatement *stat) : CAstBreakableStatement(stat) {}
public:
  py::object GetBody(void) const { return to_python(as<v8i::IterationStatement>()->body()); }

  CAstLabel GetContinueTarget(void) { return CAstLabel(as<v8i::IterationStatement>()->continue_target()); }
};

class CAstDoWhileStatement : public CAstIterationStatement
{
public:
  CAstDoWhileStatement(v8i::DoWhileStatement *stat) : CAstIterationStatement(stat) {}

  py::object GetCondition(void) const { return to_python(as<v8i::DoWhileStatement>()->cond()); }

  int GetConditionPosition(void) { return as<v8i::DoWhileStatement>()->condition_position(); }
  void SetConditionPosition(int pos) { as<v8i::DoWhileStatement>()->set_condition_position(pos); }
};

class CAstWhileStatement : public CAstIterationStatement
{
public:
  CAstWhileStatement(v8i::WhileStatement *stat) : CAstIterationStatement(stat) {}

  py::object GetCondition(void) const { return to_python(as<v8i::WhileStatement>()->cond()); }
};

class CAstForStatement : public CAstIterationStatement
{
public:
  CAstForStatement(v8i::ForStatement *stat) : CAstIterationStatement(stat) {}

  py::object GetInit(void) const { return to_python(as<v8i::ForStatement>()->init()); }
  py::object GetCondition(void) const { return to_python(as<v8i::ForStatement>()->cond()); }
  py::object GetNext(void) const { return to_python(as<v8i::ForStatement>()->next()); }

  CAstVariable GetLoopVariable(void) const { return CAstVariable(as<v8i::ForStatement>()->loop_variable()); }
  void SetLoopVariable(CAstVariable& var) { as<v8i::ForStatement>()->set_loop_variable(var.GetVariable()); }
  
  bool IsFastLoop(void) const { return as<v8i::ForStatement>()->is_fast_smi_loop(); }
};

class CAstForInStatement : public CAstIterationStatement
{
public:
  CAstForInStatement(v8i::ForInStatement *stat) : CAstIterationStatement(stat) {}

  py::object GetEach(void) const { return to_python(as<v8i::ForInStatement>()->each()); }
  py::object GetEnumerable(void) const { return to_python(as<v8i::ForInStatement>()->enumerable()); }
};

class CAstExpressionStatement : public CAstStatement
{
public:
  CAstExpressionStatement(v8i::ExpressionStatement *stat) : CAstStatement(stat) {}

  py::object GetExpression(void) const { return to_python(as<v8i::ExpressionStatement>()->expression()); }
};

class CAstContinueStatement : public CAstStatement
{
public:
  CAstContinueStatement(v8i::ContinueStatement *stat) : CAstStatement(stat) {}

  py::object GetTarget(void) const { return to_python(as<v8i::ContinueStatement>()->target()); }
};

class CAstBreakStatement : public CAstStatement
{
public:
  CAstBreakStatement(v8i::BreakStatement *stat) : CAstStatement(stat) {}

  py::object GetTarget(void) const { return to_python(as<v8i::BreakStatement>()->target()); }
};

class CAstReturnStatement : public CAstStatement
{
public:
  CAstReturnStatement(v8i::ReturnStatement *stat) : CAstStatement(stat) {}

  py::object expression(void) const { return to_python(as<v8i::ReturnStatement>()->expression()); }
};

class CAstWithStatement : public CAstStatement
{
public:
  CAstWithStatement(v8i::WithStatement *stat) : CAstStatement(stat) {}

  py::object expression(void) const { return to_python(as<v8i::WithStatement>()->expression()); }
  py::object statement(void) const { return to_python(as<v8i::WithStatement>()->statement()); }
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

  py::object tag(void) const { return to_python(as<v8i::SwitchStatement>()->tag()); }

  py::object cases(void) const { return to_python<CAstCaseClause>(as<v8i::SwitchStatement>()->cases()); }
};

class CAstIfStatement : public CAstStatement
{
public:
  CAstIfStatement(v8i::IfStatement *stat) : CAstStatement(stat) {}

  bool HasThenStatement(void) const { return as<v8i::IfStatement>()->HasThenStatement(); }
  bool HasElseStatement(void) const { return as<v8i::IfStatement>()->HasElseStatement(); }

  py::object GetCondition(void) const { return to_python(as<v8i::IfStatement>()->condition()); }

  py::object GetThenStatement(void) const { return to_python(as<v8i::IfStatement>()->then_statement()); }
  py::object GetElseStatement(void) const { return to_python(as<v8i::IfStatement>()->else_statement()); }
};

class CAstTargetCollector : public CAstNode
{
public:
  CAstTargetCollector(v8i::TargetCollector *collector) : CAstNode(collector) {}

  py::list GetTargets(void) const;
};

class CAstTryStatement : public CAstStatement
{
protected:
  CAstTryStatement(v8i::TryStatement *stat) : CAstStatement(stat) {}
public:
  CAstBlock GetTryBlock(void) const { return CAstBlock(as<v8i::TryStatement>()->try_block()); }
  py::list GetEscapingTargets(void) const;
};

class CAstTryCatchStatement : public CAstTryStatement
{
public:
  CAstTryCatchStatement(v8i::TryCatchStatement *stat) : CAstTryStatement(stat) {}

  CAstBlock GetCatchBlock(void) const { return CAstBlock(as<v8i::TryCatchStatement>()->catch_block()); }
  CAstScope GetScope(void) const { return CAstScope(as<v8i::TryCatchStatement>()->scope()); }
  CAstVariable GetVariable(void) const { return CAstVariable(as<v8i::TryCatchStatement>()->variable()); }
};

class CAstTryFinallyStatement : public CAstTryStatement
{
public:
  CAstTryFinallyStatement(v8i::TryFinallyStatement *stat) : CAstTryStatement(stat) {}

  CAstBlock GetFinallyBlock(void) const { return CAstBlock(as<v8i::TryFinallyStatement>()->finally_block()); }
};

class CAstDebuggerStatement : public CAstStatement
{
public:
  CAstDebuggerStatement(v8i::DebuggerStatement *stat) : CAstStatement(stat) {}
};

class CAstEmptyStatement : public CAstStatement
{
public:
  CAstEmptyStatement(v8i::EmptyStatement *stat) : CAstStatement(stat) {}
};

class CAstLiteral : public CAstExpression
{
public:
  CAstLiteral(v8i::Literal *lit) : CAstExpression(lit) {}

  const std::string AsPropertyName(void) const { return to_string(as<v8i::Literal>()->AsPropertyName()); }
  bool IsNull(void) const { return as<v8i::Literal>()->IsNull(); }
  bool IsTrue(void) const { return as<v8i::Literal>()->IsTrue(); }
  bool IsFalse(void) const { return as<v8i::Literal>()->IsFalse(); }
};

class CAstMaterializedLiteral : public CAstExpression
{
protected:
  CAstMaterializedLiteral(v8i::MaterializedLiteral *lit) : CAstExpression(lit) {}
public:
  int GetIndex(void) const { return as<v8i::MaterializedLiteral>()->literal_index(); }
  bool IsSimple(void) const { return as<v8i::MaterializedLiteral>()->is_simple(); }
  int GetDepth(void) const { return as<v8i::MaterializedLiteral>()->depth(); }
};

class CAstObjectProperty
{
  v8i::ObjectLiteral::Property *m_prop;
public:
  CAstObjectProperty(v8i::ObjectLiteral::Property *prop) : m_prop(prop)
  {

  }

  py::object GetKey(void) { return to_python(m_prop->key()); }
  py::object GetValue(void) { return to_python(m_prop->value()); }
  v8i::ObjectLiteral::Property::Kind GetKind(void) { return m_prop->kind(); }

  bool IsCompileTimeValue(void) { return m_prop->IsCompileTimeValue(); }
};

class CAstObjectLiteral : public CAstMaterializedLiteral
{
public:
  CAstObjectLiteral(v8i::ObjectLiteral *lit) : CAstMaterializedLiteral(lit) {}

  py::list GetProperties(void) const { return to_python<CAstObjectProperty>(as<v8i::ObjectLiteral>()->properties()); }
};

class CAstRegExpLiteral : public CAstMaterializedLiteral
{
public:
  CAstRegExpLiteral(v8i::RegExpLiteral *lit) : CAstMaterializedLiteral(lit) {}

  const std::string GetPattern(void) const { return to_string(as<v8i::RegExpLiteral>()->pattern()); }
  const std::string GetFlags(void) const { return to_string(as<v8i::RegExpLiteral>()->flags()); }
};

class CAstArrayLiteral : public CAstMaterializedLiteral
{
public:
  CAstArrayLiteral(v8i::ArrayLiteral *lit) : CAstMaterializedLiteral(lit) {}

  py::list GetValues(void) const { return to_python(as<v8i::ArrayLiteral>()->values()); }
};

class CAstVariableProxy : public CAstExpression
{
public:
  CAstVariableProxy(v8i::VariableProxy *proxy) : CAstExpression(proxy) {}

  bool IsValidLeftHandSide(void) const { return as<v8i::VariableProxy>()->IsValidLeftHandSide(); }
  bool IsArguments(void) const { return as<v8i::VariableProxy>()->IsArguments(); }
  py::object name(void) const { return to_python(as<v8i::VariableProxy>()->name()); }
  py::object var(void) const { v8i::Variable *var = as<v8i::VariableProxy>()->var(); return var ? py::object(CAstVariable(var)) : py::object();  }
  bool is_this() const  { return as<v8i::VariableProxy>()->is_this(); }
};

class CAstProperty : public CAstExpression
{
public:
  CAstProperty(v8i::Property *prop) : CAstExpression(prop) {}
};

class CAstCall : public CAstExpression
{
public:
  CAstCall(v8i::Call *call) : CAstExpression(call) {}

  py::object GetExpression(void) const { return to_python(as<v8i::Call>()->expression()); }
  py::list GetArguments(void) const { return to_python(as<v8i::Call>()->arguments()); }
  int GetPosition(void) const { return as<v8i::Call>()->position(); }
};

class CAstCallNew : public CAstExpression
{
public:
  CAstCallNew(v8i::CallNew *call) : CAstExpression(call) {}

  py::object GetExpression(void) const { return to_python(as<v8i::CallNew>()->expression()); }
  py::list GetArguments(void) const { return to_python(as<v8i::CallNew>()->arguments()); }
  int GetPosition(void) const { return as<v8i::CallNew>()->position(); }
};

class CAstCallRuntime : public CAstExpression
{
public:
  CAstCallRuntime(v8i::CallRuntime *call) : CAstExpression(call) {}

  py::object GetName(void) const { return to_python(as<v8i::CallRuntime>()->name()); }
  py::list GetArguments(void) const { return to_python(as<v8i::CallRuntime>()->arguments()); }
  bool IsJSRuntime(void) const { return as<v8i::CallRuntime>()->is_jsruntime(); }
};

class CAstUnaryOperation : public CAstExpression
{
public:
  CAstUnaryOperation(v8i::UnaryOperation *op) : CAstExpression(op) {}

  v8i::Token::Value op(void) const { return as<v8i::UnaryOperation>()->op(); }
  py::object expression(void) const { return to_python(as<v8i::UnaryOperation>()->expression()); }
};

class CAstBinaryOperation : public CAstExpression
{
public:
  CAstBinaryOperation(v8i::BinaryOperation *op) : CAstExpression(op) {}

  v8i::Token::Value op(void) const { return as<v8i::BinaryOperation>()->op(); }
  py::object left(void) const { return to_python(as<v8i::BinaryOperation>()->left()); }
  py::object right(void) const { return to_python(as<v8i::BinaryOperation>()->right()); }
  int position(void) const { return as<v8i::BinaryOperation>()->position(); }
};

class CAstCountOperation : public CAstExpression
{
public:
  CAstCountOperation(v8i::CountOperation *op) : CAstExpression(op) {}

  bool is_prefix(void) const { return as<v8i::CountOperation>()->is_prefix(); }
  bool is_postfix(void) const { return as<v8i::CountOperation>()->is_postfix(); }

  v8i::Token::Value op(void) const { return as<v8i::CountOperation>()->op(); }
  v8i::Token::Value binary_op(void) const { return as<v8i::CountOperation>()->binary_op(); }  

  py::object expression(void) const { return to_python(as<v8i::CountOperation>()->expression()); }
  int position(void) const { return as<v8i::CountOperation>()->position(); }
};

class CAstCompareOperation : public CAstExpression
{
public:
  CAstCompareOperation(v8i::CompareOperation *op) : CAstExpression(op) {}

  v8i::Token::Value op(void) const { return as<v8i::CompareOperation>()->op(); }
  py::object left(void) const { return to_python(as<v8i::CompareOperation>()->left()); }
  py::object right(void) const { return to_python(as<v8i::CompareOperation>()->right()); }
  int position(void) const { return as<v8i::CompareOperation>()->position(); }
};

class CAstConditional : public CAstExpression
{
public:
  CAstConditional(v8i::Conditional *cond) : CAstExpression(cond) {}

  py::object condition(void) const { return to_python(as<v8i::Conditional>()->condition()); }
  py::object then_expression(void) const { return to_python(as<v8i::Conditional>()->then_expression()); }
  py::object else_expression(void) const { return to_python(as<v8i::Conditional>()->else_expression()); }

  int then_expression_position(void) const { return as<v8i::Conditional>()->then_expression_position(); }
  int else_expression_position(void) const { return as<v8i::Conditional>()->else_expression_position(); }
};

class CAstAssignment : public CAstExpression
{
public:
  CAstAssignment(v8i::Assignment *assignment) : CAstExpression(assignment) {}

  v8i::Token::Value binary_op(void) const { return as<v8i::Assignment>()->binary_op(); }

  v8i::Token::Value op(void) const { return as<v8i::Assignment>()->op(); }
  py::object target(void) const { return to_python(as<v8i::Assignment>()->target()); }
  py::object value(void) const { return to_python(as<v8i::Assignment>()->value()); }
  int position(void) const { return as<v8i::Assignment>()->position(); }
  CAstBinaryOperation binary_operation(void) const { return CAstBinaryOperation(as<v8i::Assignment>()->binary_operation()); }

  int is_compound(void) const { return as<v8i::Assignment>()->is_compound(); }
};

class CAstThrow : public CAstExpression
{
public:
  CAstThrow(v8i::Throw *th) : CAstExpression(th) {}

  py::object GetException(void) const { return to_python(as<v8i::Throw>()->exception()); }
  int GetPosition(void) const { return as<v8i::Throw>()->position(); }
};

class CAstFunctionLiteral : public CAstExpression
{
public:
  CAstFunctionLiteral(v8i::FunctionLiteral *func) : CAstExpression(func) {}

  py::object GetName(void) const { return to_python(as<v8i::FunctionLiteral>()->name()); }
  CAstScope GetScope(void) const { return CAstScope(as<v8i::FunctionLiteral>()->scope()); }
  py::list GetBody(void) const { return to_python(as<v8i::FunctionLiteral>()->body()); }

  int GetStartPosition(void) const { return as<v8i::FunctionLiteral>()->start_position(); }
  int GetEndPosition(void) const { return as<v8i::FunctionLiteral>()->end_position(); }
  bool IsExpression(void) const { return as<v8i::FunctionLiteral>()->is_expression(); }

  const std::string ToAST(void) const { return v8i::AstPrinter().PrintProgram(as<v8i::FunctionLiteral>()); }
  const std::string ToJSON(void) const { return v8i::JsonAstBuilder().BuildProgram(as<v8i::FunctionLiteral>()); }
};

class CAstSharedFunctionInfoLiteral : public CAstExpression
{
public:
  CAstSharedFunctionInfoLiteral(v8i::SharedFunctionInfoLiteral *func) : CAstExpression(func) {}
};

class CAstThisFunction : public CAstExpression
{
public:
  CAstThisFunction(v8i::ThisFunction *func) : CAstExpression(func) {}
};

class CAstVisitor : public v8i::AstVisitor
{
  py::object m_handler;
public:
  CAstVisitor(py::object handler) : m_handler(handler)
  {

  }
#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { \
  if (::PyObject_HasAttrString(m_handler.ptr(), "on"#type)) { \
    py::object callback = m_handler.attr("on"#type); \
    if (::PyCallable_Check(callback.ptr())) { \
      callback(py::object(CAst##type(node))); }; } }

  AST_NODE_LIST(DECLARE_VISIT) 

#undef DECLARE_VISIT
};

struct CAstObjectCollector : public v8i::AstVisitor
{
  py::object m_obj;

#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { m_obj = py::object(CAst##type(node)); }
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT
};

template <typename T>
inline py::object to_python(T *node)
{
  if (!node) return py::object();
  
  CAstObjectCollector collector;

  node->Accept(&collector);

  return collector.m_obj;  
}

struct CAstListCollector : public v8i::AstVisitor
{
  py::list m_nodes;

#define DECLARE_VISIT(type) virtual void Visit##type(v8i::type* node) { m_nodes.append(py::object(CAst##type(node))); }
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT
};

template <typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst)
{
  if (!lst) return py::list();

  CAstListCollector collector;

  for (int i=0; i<lst->length(); i++)
  {
    lst->at(i)->Accept(&collector);
  }

  return collector.m_nodes;
}

template <typename C, typename T>
inline py::list to_python(v8i::ZoneList<T *>* lst)
{
  if (!lst) return py::list();

  py::list targets;

  for (int i=0; i<lst->length(); i++)
  {
    targets.append(C(lst->at(i)));
  }

  return targets;
}

inline py::object CAstScope::GetReceiver(void) const 
{ 
  if (m_scope->receiver())
  {
    CAstVariable proxy(m_scope->receiver());

    return to_python(proxy);
  }
  
  return py::object(); 
}
inline py::object CAstScope::GetFunction(void) const 
{ 
  if (m_scope->function())
  {
    CAstVariableProxy var(m_scope->function());

    return to_python(var);
  }

  return py::object(); 
}
inline CAstVariable CAstScope::GetParameter(int index) const 
{ 
  return CAstVariable(m_scope->parameter(index)); 
}
inline py::object CAstScope::GetArguments(void) const 
{ 
  if (m_scope->arguments())
  {
    CAstVariable var(m_scope->arguments());

    return to_python(var);
  }
  
  return py::object(); 
}

inline void CAstNode::Visit(py::object handler) { CAstVisitor(handler).Visit(m_node); }

inline CAstVariableProxy CAstDeclaration::GetProxy(void) const { return CAstVariableProxy(as<v8i::Declaration>()->proxy()); }

inline py::object CAstDeclaration::GetFunction(void) const 
{ 
  if (as<v8i::Declaration>()->fun())
  {
    CAstFunctionLiteral func(as<v8i::Declaration>()->fun());

    return to_python(func);
  }
  
  return py::object(); 
}

inline py::list CAstTargetCollector::GetTargets(void) const { return to_python<CAstLabel>(as<v8i::TargetCollector>()->targets()); }

inline py::list CAstTryStatement::GetEscapingTargets(void) const { return to_python<CAstLabel>(as<v8i::TryStatement>()->escaping_targets()); }
