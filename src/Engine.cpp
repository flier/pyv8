#include "Engine.h"

void CEngine::Expose(void)
{
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);

  py::class_<CEngine, boost::noncopyable>("JSEngine", py::init<>())
    .add_static_property("version", &CEngine::GetVersion)

    .def("compile", &CEngine::Compile)    
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

  throw CEngineException(oss.str());
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  v8::String::AsciiValue filename(message->GetScriptResourceName());
  int lineno = message->GetLineNumber();
  v8::String::AsciiValue sourceline(message->GetSourceLine());

  std::ostringstream oss;

  oss << *filename << ":" << lineno << " -> " << *sourceline;

  throw CEngineException(oss.str());
}

boost::shared_ptr<CScript> CEngine::Compile(const std::string& src)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::String> source = v8::String::New(src.c_str());
  v8::Handle<v8::Value> name = v8::Undefined();

  v8::Handle<v8::Script> script = v8::Script::Compile(source, name);

  if (script.IsEmpty())
    CWrapperException::Throw(try_catch);

  return boost::shared_ptr<CScript>(new CScript(*this, src, v8::Persistent<v8::Script>::New(script)));
}

CJavascriptObjectPtr CEngine::ExecuteScript(v8::Persistent<v8::Script>& script)
{    
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::Value> result = script->Run();

  if (result.IsEmpty())
  {
    if (try_catch.HasCaught())
      CWrapperException::Throw(try_catch);

    result = v8::Null();
  }

  return CJavascriptObjectPtr(new CJavascriptObject(result->ToObject()));
}
