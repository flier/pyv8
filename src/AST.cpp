#include "AST.h"

void CAstNode::Expose(void)
{
  py::class_<CAstNode, boost::noncopyable>("AstNode", py::no_init)
    ;

  py::class_<CAstStatement, py::bases<CAstNode> >("AstStatement", py::no_init)
    .def("__nonzero__", &CAstStatement::operator bool)

    .add_property("pos", &CAstStatement::pos)
    ;

  py::enum_<v8i::Expression::Context>("AstExpressionContext")
    .value("Uninitialized", v8i::Expression::kUninitialized)
    .value("Effect", v8i::Expression::kEffect)
    .value("Value", v8i::Expression::kValue)
    .value("Test", v8i::Expression::kTest)
    .value("ValueTest", v8i::Expression::kValueTest)
    .value("TestValue", v8i::Expression::kTestValue)
    ;
    
  py::class_<CAstExpression, py::bases<CAstNode> >("AstExpression", py::no_init)
    .add_property("context", &CAstExpression::context)
    ;

  py::class_<CAstBreakableStatement, py::bases<CAstStatement> >("AstBreakableStatement", py::no_init)
    .add_property("anonymous", &CAstBreakableStatement::anonymous)
    ;

  py::class_<CAstBlock, py::bases<CAstBreakableStatement> >("AstBlock", py::no_init)
    ;

  py::class_<CAstDeclaration, py::bases<CAstNode> >("AstDeclaration", py::no_init)
    ;

  py::class_<CAstIterationStatement, py::bases<CAstBreakableStatement> >("AstIterationStatement", py::no_init)
    ;

  py::class_<CAstDoWhileStatement, py::bases<CAstIterationStatement> >("AstDoWhileStatement", py::no_init)
    ;

  py::class_<CAstWhileStatement, py::bases<CAstIterationStatement> >("AstWhileStatement", py::no_init)
    ;

  py::class_<CAstForStatement, py::bases<CAstIterationStatement> >("AstForStatement", py::no_init)
    ;

  py::class_<CAstForInStatement, py::bases<CAstIterationStatement> >("AstForInStatement", py::no_init)
    ;

  py::class_<CAstExpressionStatement, py::bases<CAstStatement> >("AstExpressionStatement", py::no_init)
    .add_property("expression", &CAstExpressionStatement::expression)
    ;

  py::class_<CAstContinueStatement, py::bases<CAstStatement> >("AstContinueStatement", py::no_init)
    ;

  py::class_<CAstBreakStatement, py::bases<CAstStatement> >("AstBreakStatement", py::no_init)
    ;

  py::class_<CAstReturnStatement, py::bases<CAstStatement> >("AstReturnStatement", py::no_init)
    ;

  py::class_<CAstWithEnterStatement, py::bases<CAstStatement> >("AstWithEnterStatement", py::no_init)
    ;

  py::class_<CAstWithExitStatement, py::bases<CAstStatement> >("AstWithExitStatement", py::no_init)
    ;

  py::class_<CAstCaseClause>("AstCaseClause", py::no_init)
    ;

  py::class_<CAstSwitchStatement, py::bases<CAstBreakableStatement> >("AstSwitchStatement", py::no_init)
    ;

  py::class_<CAstIfStatement, py::bases<CAstStatement> >("AstIfStatement", py::no_init)
    ;

  py::class_<CAstTargetCollector, py::bases<CAstNode> >("AstTargetCollector", py::no_init)
    ;

  py::class_<CAstTryStatement, py::bases<CAstStatement> >("AstTryStatement", py::no_init)
    ;

  py::class_<CAstTryCatchStatement, py::bases<CAstTryStatement> >("AstTryCatchStatement", py::no_init)
    ;

  py::class_<CAstTryFinallyStatement, py::bases<CAstTryStatement> >("AstTryFinallyStatement", py::no_init)
    ;

  py::class_<CAstDebuggerStatement, py::bases<CAstStatement> >("AstDebuggerStatement", py::no_init)
    ;

  py::class_<CAstEmptyStatement, py::bases<CAstStatement> >("AstEmptyStatement", py::no_init)
    ;

  py::class_<CAstLiteral, py::bases<CAstExpression> >("AstLiteral", py::no_init)
    ;

  py::class_<CAstMaterializedLiteral, py::bases<CAstExpression> >("AstMaterializedLiteral", py::no_init)
    ;

  py::class_<CAstObjectLiteral, py::bases<CAstMaterializedLiteral> >("AstObjectLiteral", py::no_init)
    ;

  py::class_<CAstRegExpLiteral, py::bases<CAstMaterializedLiteral> >("AstRegExpLiteral", py::no_init)
    ;

  py::class_<CAstArrayLiteral, py::bases<CAstMaterializedLiteral> >("AstArrayLiteral", py::no_init)
    ;

  py::class_<CAstCatchExtensionObject, py::bases<CAstExpression> >("AstCatchExtensionObject", py::no_init)
    ;

  py::class_<CAstVariableProxy, py::bases<CAstExpression> >("AstVariableProxy", py::no_init)
    ;

  py::class_<CAstSlot, py::bases<CAstExpression> >("AstSlot", py::no_init)
    ;

  py::class_<CAstProperty, py::bases<CAstExpression> >("AstProperty", py::no_init)
    ;

  py::class_<CAstCall, py::bases<CAstExpression> >("AstCall", py::no_init)
    .add_property("expression", &CAstCall::expression)
    .add_property("arguments", &CAstCall::arguments)
    .add_property("position", &CAstCall::position)
    ;

  py::class_<CAstCallNew, py::bases<CAstExpression> >("AstCallNew", py::no_init)
    ;

  py::class_<CAstCallRuntime, py::bases<CAstExpression> >("AstCallRuntime", py::no_init)
    ;

  py::class_<CAstUnaryOperation, py::bases<CAstExpression> >("AstUnaryOperation", py::no_init)
    ;

  py::class_<CAstBinaryOperation, py::bases<CAstExpression> >("AstBinaryOperation", py::no_init)
    ;

  py::class_<CAstCountOperation, py::bases<CAstExpression> >("AstCountOperation", py::no_init)
    ;

  py::class_<CAstCompareOperation, py::bases<CAstExpression> >("AstCompareOperation", py::no_init)
    ;

  py::class_<CAstConditional, py::bases<CAstExpression> >("AstConditional", py::no_init)
    ;

  py::class_<CAstAssignment, py::bases<CAstExpression> >("AstAssignment", py::no_init)
    ;

  py::class_<CAstThrow, py::bases<CAstExpression> >("AstThrow", py::no_init)
    ;

  py::class_<CAstScope>("AstScope", py::no_init)
    .add_property("is_eval", &CAstScope::is_eval)
    .add_property("is_func", &CAstScope::is_func)
    .add_property("is_global", &CAstScope::is_global)

    .add_property("calls_eval", &CAstScope::calls_eval)
    .add_property("outer_scope_calls_eval", &CAstScope::outer_scope_calls_eval)

    .add_property("inside_with", &CAstScope::inside_with)
    .add_property("contains_with", &CAstScope::contains_with)

    .add_property("outer", &CAstScope::outer)

    .add_property("declarations", &CAstScope::declarations)
    ;

  py::class_<CAstFunctionLiteral, py::bases<CAstExpression> >("AstFunctionLiteral", py::no_init)
    .add_property("name", &CAstFunctionLiteral::name)
    .add_property("scope", &CAstFunctionLiteral::scope)    
    .add_property("body", &CAstFunctionLiteral::body)

    .add_property("start_pos", &CAstFunctionLiteral::start_position)
    .add_property("end_pos", &CAstFunctionLiteral::end_position)
    .add_property("is_expression", &CAstFunctionLiteral::is_expression)    
    ;

  py::class_<CAstFunctionBoilerplateLiteral, py::bases<CAstExpression> >("AstFunctionBoilerplateLiteral", py::no_init)
    ;

  py::class_<CAstThisFunction, py::bases<CAstExpression> >("AstThisFunction", py::no_init)
    ;
}

const std::string CAstFunctionLiteral::name(void) const 
{ 
  v8i::Handle<v8i::String> name = as<v8i::FunctionLiteral>()->name();
  v8i::Vector<const char> str = name->ToAsciiVector();

  return std::string(str.start(), str.length());
}
