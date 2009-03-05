#include "Exception.h"

#include <sstream>

void CPythonException::Expose(void)
{
  py::register_exception_translator<CEngineException>(CPythonException::translator);
  py::register_exception_translator<CWrapperException>(CPythonException::translator);
}

void CPythonException::translator(CPythonException const& ex) 
{
  ::PyErr_SetString(ex.m_exc, ex.what());
}

CExceptionExtractor::CExceptionExtractor(v8::TryCatch& try_catch)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::String::AsciiValue msg(try_catch.Exception());

  std::ostringstream oss;

  oss << std::string(*msg, msg.length());

  v8::Handle<v8::Message> message = try_catch.Message();

  if (!message.IsEmpty())
  {
    oss << " ( ";

    if (!message->GetScriptResourceName().IsEmpty() &&
        !message->GetScriptResourceName()->IsUndefined())
    {
      v8::String::AsciiValue name(message->GetScriptResourceName());

      oss << std::string(*name, name.length());
    }

    oss << " @ " << message->GetLineNumber() << " : " << message->GetStartColumn() << " ) ";
    
    if (!message->GetSourceLine().IsEmpty() &&
        !message->GetSourceLine()->IsUndefined())
    {
      v8::String::AsciiValue line(message->GetSourceLine());

      oss << " -> " << std::string(*line, line.length());
    }
  }

  m_msg = oss.str();
}