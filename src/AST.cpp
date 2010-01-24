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
    .def("accept", &CAstBlock::Accept, (py::arg("callback")))
    ;

  py::class_<CAstDeclaration, py::bases<CAstNode> >("AstDeclaration", py::no_init)
    .def("accept", &CAstDeclaration::Accept, (py::arg("callback")))
    ;

  py::class_<CAstIterationStatement, py::bases<CAstBreakableStatement> >("AstIterationStatement", py::no_init)
    ;

  py::class_<CAstDoWhileStatement, py::bases<CAstIterationStatement> >("AstDoWhileStatement", py::no_init)
    .def("accept", &CAstDoWhileStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstWhileStatement, py::bases<CAstIterationStatement> >("AstWhileStatement", py::no_init)
    .def("accept", &CAstWhileStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstForStatement, py::bases<CAstIterationStatement> >("AstForStatement", py::no_init)
    .def("accept", &CAstForStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstForInStatement, py::bases<CAstIterationStatement> >("AstForInStatement", py::no_init)
    .def("accept", &CAstForInStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstExpressionStatement, py::bases<CAstStatement> >("AstExpressionStatement", py::no_init)
    .def("accept", &CAstExpressionStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstContinueStatement, py::bases<CAstStatement> >("AstContinueStatement", py::no_init)
    .def("accept", &CAstContinueStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstBreakStatement, py::bases<CAstStatement> >("AstBreakStatement", py::no_init)
    .def("accept", &CAstBreakStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstReturnStatement, py::bases<CAstStatement> >("AstReturnStatement", py::no_init)
    .def("accept", &CAstReturnStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstWithEnterStatement, py::bases<CAstStatement> >("AstWithEnterStatement", py::no_init)
    .def("accept", &CAstWithEnterStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstWithExitStatement, py::bases<CAstStatement> >("AstWithExitStatement", py::no_init)
    .def("accept", &CAstWithExitStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCaseClause>("AstCaseClause", py::no_init)
    ;

  py::class_<CAstSwitchStatement, py::bases<CAstBreakableStatement> >("AstSwitchStatement", py::no_init)
    .def("accept", &CAstSwitchStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstIfStatement, py::bases<CAstStatement> >("AstIfStatement", py::no_init)
    .def("accept", &CAstIfStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstTargetCollector, py::bases<CAstNode> >("AstTargetCollector", py::no_init)
    ;

  py::class_<CAstTryStatement, py::bases<CAstStatement> >("AstTryStatement", py::no_init)
    ;

  py::class_<CAstTryCatchStatement, py::bases<CAstTryStatement> >("AstTryCatchStatement", py::no_init)
    .def("accept", &CAstTryCatchStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstTryFinallyStatement, py::bases<CAstTryStatement> >("AstTryFinallyStatement", py::no_init)
    .def("accept", &CAstTryFinallyStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstDebuggerStatement, py::bases<CAstStatement> >("AstDebuggerStatement", py::no_init)
    .def("accept", &CAstDebuggerStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstEmptyStatement, py::bases<CAstStatement> >("AstEmptyStatement", py::no_init)
    .def("accept", &CAstEmptyStatement::Accept, (py::arg("callback")))
    ;

  py::class_<CAstLiteral, py::bases<CAstExpression> >("AstLiteral", py::no_init)
    .def("accept", &CAstLiteral::Accept, (py::arg("callback")))
    ;

  py::class_<CAstMaterializedLiteral, py::bases<CAstExpression> >("AstMaterializedLiteral", py::no_init)
    ;

  py::class_<CAstObjectLiteral, py::bases<CAstMaterializedLiteral> >("AstObjectLiteral", py::no_init)
    .def("accept", &CAstObjectLiteral::Accept, (py::arg("callback")))
    ;

  py::class_<CAstRegExpLiteral, py::bases<CAstMaterializedLiteral> >("AstRegExpLiteral", py::no_init)
    .def("accept", &CAstRegExpLiteral::Accept, (py::arg("callback")))
    ;

  py::class_<CAstArrayLiteral, py::bases<CAstMaterializedLiteral> >("AstArrayLiteral", py::no_init)
    .def("accept", &CAstArrayLiteral::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCatchExtensionObject, py::bases<CAstExpression> >("AstCatchExtensionObject", py::no_init)
    .def("accept", &CAstCatchExtensionObject::Accept, (py::arg("callback")))
    ;

  py::class_<CAstVariableProxy, py::bases<CAstExpression> >("AstVariableProxy", py::no_init)
    .def("accept", &CAstVariableProxy::Accept, (py::arg("callback")))
    ;

  py::class_<CAstSlot, py::bases<CAstExpression> >("AstSlot", py::no_init)
    .def("accept", &CAstSlot::Accept, (py::arg("callback")))
    ;

  py::class_<CAstProperty, py::bases<CAstExpression> >("AstProperty", py::no_init)
    .def("accept", &CAstProperty::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCall, py::bases<CAstExpression> >("AstCall", py::no_init)
    .def("accept", &CAstCall::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCallNew, py::bases<CAstExpression> >("AstCallNew", py::no_init)
    .def("accept", &CAstCallNew::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCallRuntime, py::bases<CAstExpression> >("AstCallRuntime", py::no_init)
    .def("accept", &CAstCallRuntime::Accept, (py::arg("callback")))
    ;

  py::class_<CAstUnaryOperation, py::bases<CAstExpression> >("AstUnaryOperation", py::no_init)
    .def("accept", &CAstUnaryOperation::Accept, (py::arg("callback")))
    ;

  py::class_<CAstBinaryOperation, py::bases<CAstExpression> >("AstBinaryOperation", py::no_init)
    .def("accept", &CAstBinaryOperation::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCountOperation, py::bases<CAstExpression> >("AstCountOperation", py::no_init)
    .def("accept", &CAstCountOperation::Accept, (py::arg("callback")))
    ;

  py::class_<CAstCompareOperation, py::bases<CAstExpression> >("AstCompareOperation", py::no_init)
    .def("accept", &CAstCompareOperation::Accept, (py::arg("callback")))
    ;

  py::class_<CAstConditional, py::bases<CAstExpression> >("AstConditional", py::no_init)
    .def("accept", &CAstConditional::Accept, (py::arg("callback")))
    ;

  py::class_<CAstAssignment, py::bases<CAstExpression> >("AstAssignment", py::no_init)
    .def("accept", &CAstAssignment::Accept, (py::arg("callback")))
    ;

  py::class_<CAstThrow, py::bases<CAstExpression> >("AstThrow", py::no_init)
    .def("accept", &CAstThrow::Accept, (py::arg("callback")))
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
    .def("accept", &CAstFunctionLiteral::Accept, (py::arg("callback")))

    .add_property("name", &CAstFunctionLiteral::name)
    .add_property("scope", &CAstFunctionLiteral::scope)    
    .add_property("body", &CAstFunctionLiteral::body)

    .add_property("start_pos", &CAstFunctionLiteral::start_position)
    .add_property("end_pos", &CAstFunctionLiteral::end_position)
    .add_property("is_expression", &CAstFunctionLiteral::is_expression)    
    ;

  py::class_<CAstFunctionBoilerplateLiteral, py::bases<CAstExpression> >("AstFunctionBoilerplateLiteral", py::no_init)
    .def("accept", &CAstFunctionBoilerplateLiteral::Accept, (py::arg("callback")))
    ;

  py::class_<CAstThisFunction, py::bases<CAstExpression> >("AstThisFunction", py::no_init)
    .def("accept", &CAstThisFunction::Accept, (py::arg("callback")))
    ;
}

#define DEFINE_ACCEPT(type) void CAst##type::Accept(py::object callback) { CAstVisitor visitor(callback); as<v8i::type>()->Accept(&visitor); }

AST_NODE_LIST(DEFINE_ACCEPT)

const std::string CAstFunctionLiteral::name(void) const 
{ 
  v8i::Vector<const char> str = as<v8i::FunctionLiteral>()->name()->ToAsciiVector();

  return std::string(str.start(), str.length());
}
