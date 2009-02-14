#include "StdAfx.h"
#include "Engine.h"

CEngine::CEngine(py::object global) 
{
  v8::HandleScope handle_scope;

  m_context = v8::Context::New(NULL, v8::ObjectTemplate::New());

  v8::Context::Scope context_scope(m_context);
  
  m_wrapper.Attach(*m_context);

  m_context->Global()->Set(v8::String::New("__proto__"), m_wrapper.Wrap(global));  
}

void CEngine::Expose(void)
{
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);

  py::register_exception_translator<CEngineException>(CPythonException::translator);
  py::register_exception_translator<CWrapperException>(CPythonException::translator);

  py::class_<CEngine, boost::noncopyable>("Engine", py::no_init)
    .def(py::init< py::optional<py::object> >(py::args("global")))

    .add_static_property("version", &CEngine::GetVersion)

    .def("compile", &CEngine::Compile)
    .def("eval", &CEngine::Execute)
    ;
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
  std::ostringstream oss;

  oss << "<" << location << "> " << message;

  throw CEngineException(oss.str());
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  UNREFERENCED_PARAMETER(data);

  v8::String::AsciiValue filename(message->GetScriptResourceName());
  int lineno = message->GetLineNumber();
  v8::String::AsciiValue sourceline(message->GetSourceLine());

  std::ostringstream oss;

  oss << *filename << ":" << lineno << " -> " << *sourceline;

  ::PyErr_SetString(::PyExc_UserWarning, oss.str().c_str());
}

boost::shared_ptr<CScript> CEngine::Compile(const std::string& src)
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::TryCatch try_catch;

  v8::Handle<v8::String> source = v8::String::New(src.c_str());
  v8::Handle<v8::Value> name = v8::Undefined();

  v8::Handle<v8::Script> script = v8::Script::Compile(source, name);

  if (script.IsEmpty()) throw CEngineException(try_catch);

  return boost::shared_ptr<CScript>(new CScript(*this, src, v8::Persistent<v8::Script>::New(script)));
}

py::object CEngine::Execute(const std::string& src)
{
  return Compile(src)->Run();
}

py::object CEngine::ExecuteScript(v8::Persistent<v8::Script>& script)
{    
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::TryCatch try_catch;

  v8::Handle<v8::Value> result = script->Run();

  if (result.IsEmpty()) throw CEngineException(try_catch);

  if (result->IsUndefined()) return py::object();

  return py::str(std::string(*v8::String::AsciiValue(result)));
}

void CScript::Expose(void)
{
  py::objects::class_value_wrapper<boost::shared_ptr<CScript>, 
    py::objects::make_ptr_instance<CScript, 
    py::objects::pointer_holder<boost::shared_ptr<CScript>,CScript> > >();

  py::class_<CScript, boost::noncopyable>("Script", py::no_init)
    .add_property("source", &CScript::GetSource)

    .def("run", &CScript::Run)
    ;
}