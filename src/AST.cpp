#include "AST.h"

void CAstNode::Expose(void)
{
  py::class_<CAstScope>("AstScope", py::no_init)
    .add_property("isEval", &CAstScope::is_eval)
    .add_property("isFunc", &CAstScope::is_func)
    .add_property("isGlobal", &CAstScope::is_global)

    .add_property("callsEval", &CAstScope::calls_eval)
    .add_property("outerScopeCallsEval", &CAstScope::outer_scope_calls_eval)

    .add_property("insideWith", &CAstScope::inside_with)
    .add_property("containsWith", &CAstScope::contains_with)

    .add_property("outer", &CAstScope::outer)

    .add_property("declarations", &CAstScope::declarations)
    
    .add_property("num_parameters", &CAstScope::num_parameters)
    .def("parameter", &CAstScope::parameter, (py::args("index")))
    ;

  py::enum_<v8i::Variable::Mode>("AstVariableMode")
    .value("VAR", v8i::Variable::VAR)
    .value("CONST", v8i::Variable::CONST)
    .value("DYNAMIC", v8i::Variable::DYNAMIC)
    .value("DYNAMIC_GLOBAL", v8i::Variable::DYNAMIC_GLOBAL)
    .value("DYNAMIC_LOCAL", v8i::Variable::DYNAMIC_LOCAL)
    .value("INTERNAL", v8i::Variable::INTERNAL)
    .value("TEMPORARY", v8i::Variable::TEMPORARY)
    ;

  py::class_<CAstVariable>("AstVariable", py::no_init)
    .add_property("scope", &CAstVariable::scope)
    .add_property("name", &CAstVariable::name)
    .add_property("mode", &CAstVariable::mode)
    .add_property("isValidLeftHandSide", &CAstVariable::IsValidLeftHandSide)
    .add_property("isGlobal", &CAstVariable::is_global)
    .add_property("isThis", &CAstVariable::is_this)
    .add_property("isArguments", &CAstVariable::is_arguments)
    .add_property("isPossiblyEval", &CAstVariable::is_possibly_eval)
    ;

  py::class_<CAstLabel>("AstLabel", py::no_init)
    .add_property("pos", &CAstLabel::GetPosition)
    .add_property("bound", &CAstLabel::IsBound)
    .add_property("unused", &CAstLabel::IsUnused)
    .add_property("linked", &CAstLabel::IsLinked)
    ;

  py::class_<CAstJumpTarget>("AstJumpTarget", py::no_init)
    .add_property("entryLabel", &CAstJumpTarget::GetEntryLabel)

    .add_property("bound", &CAstJumpTarget::IsBound)
    .add_property("unused", &CAstJumpTarget::IsUnused)
    .add_property("linked", &CAstJumpTarget::IsLinked)
    ;

  py::class_<CAstBreakTarget, py::bases<CAstJumpTarget> >("AstBreakTarget", py::no_init)
    ;

  #define DECLARE_TYPE_ENUM(type) .value(#type, v8i::AstNode::k##type)

  py::enum_<v8i::AstNode::Type>("AstNodeType")
    AST_NODE_LIST(DECLARE_TYPE_ENUM)

    .value("Invalid", v8i::AstNode::kInvalid)    
    ;

  py::class_<CAstNode, boost::noncopyable>("AstNode", py::no_init)
    .add_property("type", &CAstNode::GetType)

    .def("visit", &CAstNode::Visit, (py::arg("handler")))

    .def("__str__", &CAstNode::ToString)
    ;

  py::class_<CAstStatement, py::bases<CAstNode> >("AstStatement", py::no_init)
    .def("__nonzero__", &CAstStatement::operator bool)

    .add_property("pos", &CAstStatement::GetPosition, &CAstStatement::SetPosition)
    ;
    
  py::class_<CAstExpression, py::bases<CAstNode> >("AstExpression", py::no_init)    
    .add_property("trivial", &CAstExpression::IsTrivial)
    .add_property("propertyName", &CAstExpression::IsPropertyName)

    .add_property("loopCondition", &CAstExpression::IsLoopCondition, &CAstExpression::SetLoopCondition)
    ;

  py::class_<CAstBreakableStatement, py::bases<CAstStatement> >("AstBreakableStatement", py::no_init)
    .add_property("anonymous", &CAstBreakableStatement::IsTargetForAnonymous)
    .add_property("breakTarget", &CAstBreakableStatement::GetBreakTarget)
    ;

  py::class_<CAstBlock, py::bases<CAstBreakableStatement> >("AstBlock", py::no_init)
    .add_property("statements", &CAstBlock::GetStatements)
    .add_property("initializerBlock", &CAstBlock::IsInitializerBlock)

    .def("addStatement", &CAstBlock::AddStatement)
    ;

  py::class_<CAstDeclaration, py::bases<CAstNode> >("AstDeclaration", py::no_init)
    ;

  py::class_<CAstIterationStatement, py::bases<CAstBreakableStatement> >("AstIterationStatement", py::no_init)
    .add_property("body", &CAstIterationStatement::GetBody, &CAstIterationStatement::SetBody)
    .add_property("continueTarget", &CAstIterationStatement::GetContinueTarget)
    ;

  py::class_<CAstDoWhileStatement, py::bases<CAstIterationStatement> >("AstDoWhileStatement", py::no_init)
    .add_property("condition", &CAstDoWhileStatement::GetCondition)
    .add_property("conditionPos", &CAstDoWhileStatement::GetConditionPosition, &CAstDoWhileStatement::SetConditionPosition)
    ;

  py::class_<CAstWhileStatement, py::bases<CAstIterationStatement> >("AstWhileStatement", py::no_init)
    .add_property("condition", &CAstWhileStatement::GetCondition)
    ;

  py::class_<CAstForStatement, py::bases<CAstIterationStatement> >("AstForStatement", py::no_init)
    .add_property("init", &CAstForStatement::GetInit, &CAstForStatement::SetInit)
    .add_property("condition", &CAstForStatement::GetCondition, &CAstForStatement::SetCondition)
    .add_property("next", &CAstForStatement::GetNext, &CAstForStatement::SetNext)
    .add_property("fastLoop", &CAstForStatement::IsFastLoop)
    ;

  py::class_<CAstForInStatement, py::bases<CAstIterationStatement> >("AstForInStatement", py::no_init)
    .add_property("each", &CAstForInStatement::GetEach)
    .add_property("enumerable", &CAstForInStatement::GetEnumerable)
    ;

  py::class_<CAstExpressionStatement, py::bases<CAstStatement> >("AstExpressionStatement", py::no_init)
    .add_property("expression", &CAstExpressionStatement::GetExpression)
    ;

  py::class_<CAstContinueStatement, py::bases<CAstStatement> >("AstContinueStatement", py::no_init)
    ;

  py::class_<CAstBreakStatement, py::bases<CAstStatement> >("AstBreakStatement", py::no_init)
    ;

  py::class_<CAstReturnStatement, py::bases<CAstStatement> >("AstReturnStatement", py::no_init)
    .add_property("expression", &CAstReturnStatement::expression)
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
    .add_property("hasThenStatement", &CAstIfStatement::HasThenStatement)
    .add_property("hasElseStatement", &CAstIfStatement::HasElseStatement)

    .add_property("condition", &CAstIfStatement::GetCondition)

    .add_property("thenStatement", &CAstIfStatement::GetThenStatement, &CAstIfStatement::SetThenStatement)
    .add_property("elseStatement", &CAstIfStatement::GetElseStatement, &CAstIfStatement::SetElseStatement)
    ;

  py::class_<CAstTargetCollector, py::bases<CAstNode> >("AstTargetCollector", py::no_init)
    ;

  py::class_<CAstTryStatement, py::bases<CAstStatement> >("AstTryStatement", py::no_init)
    .add_property("tryBlock", &CAstTryStatement::GetTryBlock)
    .add_property("targets", &CAstTryStatement::GetEscapingTargets)
    ;

  py::class_<CAstTryCatchStatement, py::bases<CAstTryStatement> >("AstTryCatchStatement", py::no_init)
    .add_property("catchVar", &CAstTryCatchStatement::GetCatchVariable)
    .add_property("catchBlock", &CAstTryCatchStatement::GetCatchBlock)
    ;

  py::class_<CAstTryFinallyStatement, py::bases<CAstTryStatement> >("AstTryFinallyStatement", py::no_init)
    .add_property("finallyBlock", &CAstTryFinallyStatement::GetFinallyBlock)
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
    .add_property("isValidLeftHandSide", &CAstVariableProxy::IsValidLeftHandSide)
    .add_property("isArguments", &CAstVariableProxy::IsArguments)
    .add_property("name", &CAstVariableProxy::name)
    .add_property("var", &CAstVariableProxy::var)
    .add_property("isThis", &CAstVariableProxy::is_this)
    .add_property("insideWith", &CAstVariableProxy::inside_with)
    ;

  py::class_<CAstSlot, py::bases<CAstExpression> >("AstSlot", py::no_init)
    ;

  py::class_<CAstProperty, py::bases<CAstExpression> >("AstProperty", py::no_init)
    ;

  py::class_<CAstCall, py::bases<CAstExpression> >("AstCall", py::no_init)
    .add_property("expression", &CAstCall::GetExpression)
    .add_property("args", &CAstCall::GetArguments)
    .add_property("pos", &CAstCall::GetPosition)
    ;

  py::class_<CAstCallNew, py::bases<CAstExpression> >("AstCallNew", py::no_init)
    .add_property("expression", &CAstCallNew::GetExpression)
    .add_property("args", &CAstCallNew::GetArguments)
    .add_property("pos", &CAstCallNew::GetPosition)
    ;

  py::class_<CAstCallRuntime, py::bases<CAstExpression> >("AstCallRuntime", py::no_init)
    .add_property("name", &CAstCallRuntime::GetName)
    .add_property("args", &CAstCallRuntime::GetArguments)
    .add_property("isJsRuntime", &CAstCallRuntime::IsJSRuntime)
    ;

  py::enum_<v8i::Token::Value>("AstOperation")
  #define T(name, string, precedence) .value(#name, v8i::Token::name)
    TOKEN_LIST(T, T, IGNORE_TOKEN)
  #undef T
    ;

  py::class_<CAstUnaryOperation, py::bases<CAstExpression> >("AstUnaryOperation", py::no_init)
    .add_property("op", &CAstUnaryOperation::op)
    .add_property("expression", &CAstUnaryOperation::expression)
    ;

  py::class_<CAstBinaryOperation, py::bases<CAstExpression> >("AstBinaryOperation", py::no_init)
    .add_property("op", &CAstBinaryOperation::op)
    .add_property("left", &CAstBinaryOperation::left)
    .add_property("right", &CAstBinaryOperation::right)
    ;

  py::class_<CAstCountOperation, py::bases<CAstExpression> >("AstCountOperation", py::no_init)
    .add_property("isPrefix", &CAstCountOperation::is_prefix)
    .add_property("isPostfix", &CAstCountOperation::is_postfix)

    .add_property("op", &CAstCountOperation::op)
    .add_property("binaryOp", &CAstCountOperation::binary_op)
    .add_property("expression", &CAstCountOperation::expression)
    ;

  py::class_<CAstCompareOperation, py::bases<CAstExpression> >("AstCompareOperation", py::no_init)
    .add_property("op", &CAstBinaryOperation::op)
    .add_property("left", &CAstBinaryOperation::left)
    .add_property("right", &CAstBinaryOperation::right)
    ;

  py::class_<CAstConditional, py::bases<CAstExpression> >("AstConditional", py::no_init)
    ;

  py::class_<CAstAssignment, py::bases<CAstExpression> >("AstAssignment", py::no_init)
    ;

  py::class_<CAstThrow, py::bases<CAstExpression> >("AstThrow", py::no_init)
    .add_property("exception", &CAstThrow::GetException)
    .add_property("pos", &CAstThrow::GetPosition)
    ;

  py::class_<CAstFunctionLiteral, py::bases<CAstExpression> >("AstFunctionLiteral", py::no_init)
    .add_property("name", &CAstFunctionLiteral::GetName)
    .add_property("scope", &CAstFunctionLiteral::GetScope)    
    .add_property("body", &CAstFunctionLiteral::GetBody)

    .add_property("startPos", &CAstFunctionLiteral::start_position)
    .add_property("endPos", &CAstFunctionLiteral::end_position)
    .add_property("isExpression", &CAstFunctionLiteral::is_expression)    
    ;

  py::class_<CAstSharedFunctionInfoLiteral, py::bases<CAstExpression> >("AstSharedFunctionInfoLiteral", py::no_init)
    ;

  py::class_<CAstThisFunction, py::bases<CAstExpression> >("AstThisFunction", py::no_init)
    ;
}
