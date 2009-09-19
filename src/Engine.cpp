#include "Engine.h"

void CEngine::Expose(void)
{
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);

  py::class_<CEngine, boost::noncopyable>("JSEngine", py::init<>())
    .add_static_property("version", &CEngine::GetVersion)

    .def("compile", &CEngine::Compile, (py::arg("source"), 
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1))    
    ;

  py::class_<CScript, boost::noncopyable>("JSScript", py::no_init)
    .add_property("source", &CScript::GetSource)

    .def("run", &CScript::Run)
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CScript>, 
    py::objects::make_ptr_instance<CScript, 
    py::objects::pointer_holder<boost::shared_ptr<CScript>,CScript> > >();
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
  std::ostringstream oss;

  oss << "<" << location << "> " << message;

  throw CJavascriptException(oss.str());
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  v8::String::AsciiValue filename(message->GetScriptResourceName());
  int lineno = message->GetLineNumber();
  v8::String::AsciiValue sourceline(message->GetSourceLine());

  std::ostringstream oss;

  oss << *filename << ":" << lineno << " -> " << *sourceline;

  throw CJavascriptException(oss.str());
}

boost::shared_ptr<CScript> CEngine::Compile(const std::string& src, 
                                            const std::string name,
                                            int line, int col)
{
  if (!v8::Context::InContext())
  {
    throw CJavascriptException("please enter a context first");
  }

  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::String> script_source = v8::String::New(src.c_str());
  v8::Handle<v8::Value> script_name = name.empty() ? v8::Undefined() : v8::String::New(name.c_str());

  v8::Handle<v8::Script> script;

  Py_BEGIN_ALLOW_THREADS

  if (line >= 0 && col >= 0)
  {
    v8::ScriptOrigin script_origin(script_name, v8::Integer::New(line), v8::Integer::New(col));

    script = v8::Script::Compile(script_source, &script_origin);
  }
  else
  {
    script = v8::Script::Compile(script_source, script_name);
  }

  Py_END_ALLOW_THREADS 

  if (script.IsEmpty()) CJavascriptException::ThrowIf(try_catch);

  return boost::shared_ptr<CScript>(new CScript(*this, src, script));
}

py::object CEngine::ExecuteScript(v8::Handle<v8::Script> script)
{    
  if (!v8::Context::InContext())
  {
    throw CJavascriptException("please enter a context first");
  }

  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

  result = script->Run();

  Py_END_ALLOW_THREADS

  if (result.IsEmpty())
  {
    if (try_catch.HasCaught()) CJavascriptException::ThrowIf(try_catch);

    result = v8::Null();
  }

  return CJavascriptObject::Wrap(result);
}

py::object CScript::Run(void) 
{ 
  v8::HandleScope handle_scope;

  return m_engine.ExecuteScript(m_script); 
}
