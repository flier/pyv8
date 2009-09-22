#include "Engine.h"

void CEngine::Expose(void)
{
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);

  void (*terminateThread)(int) = &v8::V8::TerminateExecution;
  void (*terminateAllThreads)(void) = &v8::V8::TerminateExecution;

  py::class_<CEngine, boost::noncopyable>("JSEngine", py::init<>())
    .add_static_property("version", &CEngine::GetVersion)

    .add_static_property("dead", &v8::V8::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .add_static_property("currentThreadId", &v8::V8::GetCurrentThreadId,
                         "the V8 thread id of the calling thread.")

    .def("terminateThread", terminateThread,
         "Forcefully terminate execution of a JavaScript thread.")
    .staticmethod("terminateThread")

    .def("terminateAllThreads", terminateAllThreads,
         "Forcefully terminate the current thread of JavaScript execution.")
    .staticmethod("terminateAllThreads")

    .def("dispose", &v8::V8::Dispose,
         "Releases any resources used by v8 and stops any utility threads "
         "that may be running. Note that disposing v8 is permanent, "
         "it cannot be reinitialized.")
    .staticmethod("dispose")

    .def("idle", &v8::V8::IdleNotification,
         "Optional notification that the embedder is idle.")
    .staticmethod("idle")

    .def("lowMemory", &v8::V8::LowMemoryNotification,
         "Optional notification that the system is running low on memory.")
    .staticmethod("lowMemory")

    .def("precompile", &CEngine::PreCompile, (py::arg("source")))
    .def("compile", &CEngine::Compile, (py::arg("source"), 
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1,
                                        py::arg("precompiled") = py::object()))    
    ;

  py::class_<CScript, boost::noncopyable>("JSScript", py::no_init)
    .add_property("source", &CScript::GetSource)

    .def("run", &CScript::Run)
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CScript>, 
    py::objects::make_ptr_instance<CScript, 
    py::objects::pointer_holder<boost::shared_ptr<CScript>, CScript> > >();
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

py::object CEngine::PreCompile(const std::string& src)
{
  v8::TryCatch try_catch;

  std::auto_ptr<v8::ScriptData> precompiled;

  Py_BEGIN_ALLOW_THREADS

  precompiled.reset(v8::ScriptData::PreCompile(src.c_str(), src.size()));

  Py_END_ALLOW_THREADS 

  if (!precompiled.get()) CJavascriptException::ThrowIf(try_catch);

  py::object obj(py::handle<>(::PyBuffer_New(sizeof(unsigned) * precompiled->Length())));

  void *buf = NULL;
  Py_ssize_t len = 0;

  if (0 == ::PyObject_AsWriteBuffer(obj.ptr(), &buf, &len) && buf && len > 0)
  {
    memcpy(buf, precompiled->Data(), len);    
  }
  else
  {
    obj = py::object();
  }

  return obj;
}

boost::shared_ptr<CScript> CEngine::Compile(const std::string& src, 
                                            const std::string name,
                                            int line, int col,
                                            py::object precompiled)
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
  std::auto_ptr<v8::ScriptData> script_data;

  if (PyBuffer_Check(precompiled.ptr()))
  {
    const void *buf = NULL;
    Py_ssize_t len = 0;

    if (0 == ::PyObject_AsReadBuffer(precompiled.ptr(), &buf, &len) && buf && len > 0)
    {
      script_data.reset(v8::ScriptData::New((unsigned *)buf, len/sizeof(unsigned)));
    }
  }

  Py_BEGIN_ALLOW_THREADS

  if (line >= 0 && col >= 0)
  {
    v8::ScriptOrigin script_origin(script_name, v8::Integer::New(line), v8::Integer::New(col));

    script = v8::Script::Compile(script_source, &script_origin, script_data.release());
  }
  else
  {
    v8::ScriptOrigin script_origin(script_name);

    script = v8::Script::Compile(script_source, &script_origin, script_data.release());
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
