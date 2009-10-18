#include "Engine.h"

#undef COMPILER

#include "src/v8.h"

#include "src/bootstrapper.h"
#include "src/natives.h"
#include "src/platform.h"
#include "src/serialize.h"
#include "src/stub-cache.h"
#include "src/heap.h"

CEngine::CounterTable CEngine::m_counters;

void CEngine::Expose(void)
{
  //v8::V8::Initialize();
  //v8::V8::SetFatalErrorHandler(ReportFatalError);
  //v8::V8::AddMessageListener(ReportMessage);

  void (*terminateThread)(int) = &v8::V8::TerminateExecution;
  void (*terminateAllThreads)(void) = &v8::V8::TerminateExecution;

  py::class_<CEngine, boost::noncopyable>("JSEngine", py::init<>())
    .add_static_property("version", &CEngine::GetVersion)

    .add_static_property("dead", &v8::V8::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .add_static_property("currentThreadId", &v8::V8::GetCurrentThreadId,
                         "the V8 thread id of the calling thread.")

    .add_static_property("serializeEnabled", &CEngine::IsSerializeEnabled, &CEngine::SetSerializeEnable)

    .def("serialize", &CEngine::Serialize)
    .staticmethod("serialize")

    .def("deserialize", &CEngine::Deserialize)
    .staticmethod("deserialize")

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

  py::class_<CExtension, boost::noncopyable>("JSExtension", py::no_init)    
    .def(py::init<const std::string&, const std::string&, py::object, py::list, bool>((py::arg("name"), 
                                                                                       py::arg("source"),
                                                                                       py::arg("callback") = py::object(),
                                                                                       py::arg("dependencies") = py::list(),
                                                                                       py::arg("register") = true)))
    .add_property("name", &CExtension::GetName)
    .add_property("source", &CExtension::GetSource)
    .add_property("dependencies", &CExtension::GetDependencies)

    .add_property("autoEnable", &CExtension::IsAutoEnable, &CExtension::SetAutoEnable)

    .add_property("registered", &CExtension::IsRegistered)
    .def("register", &CExtension::Register)
    ;
}

int *CEngine::CounterLookup(const char* name)
{
  CounterTable::const_iterator it = m_counters.find(name);

  if (it == m_counters.end())
    m_counters[name] = 0;

  return &m_counters[name];
}

void CEngine::SetSerializeEnable(bool value)
{
  if (value)
    v8::internal::Serializer::Enable();
  else
    v8::internal::Serializer::Disable();
}
bool CEngine::IsSerializeEnabled(void)
{
  return v8::internal::Serializer::enabled();
}

py::object CEngine::Serialize(void)
{
  v8::internal::byte* data = NULL;
  int size = 0;

  Py_BEGIN_ALLOW_THREADS

  v8::internal::StatsTable::SetCounterFunction(&CEngine::CounterLookup);

  v8::internal::Serializer serializer;

  serializer.Serialize();

  serializer.Finalize(&data, &size);

  Py_END_ALLOW_THREADS 

  py::object obj(py::handle<>(::PyBuffer_New(size)));

  void *buf = NULL;
  Py_ssize_t len = 0;

  if (0 == ::PyObject_AsWriteBuffer(obj.ptr(), &buf, &len) && buf && len > 0)
  {
    memcpy(buf, data, len);    
  }
  else
  {
    obj = py::object();
  }

  return obj;
}
void CEngine::Deserialize(py::object snapshot)
{
  const void *buf = NULL;
  Py_ssize_t len = 0;

  if (PyBuffer_Check(snapshot.ptr()))
  {
    if (0 != ::PyObject_AsReadBuffer(snapshot.ptr(), &buf, &len))
    {
      buf = NULL;
    }
  }
  else if(PyString_CheckExact(snapshot.ptr()) || PyUnicode_CheckExact(snapshot.ptr()))
  {
    if (0 != ::PyString_AsStringAndSize(snapshot.ptr(), (char **)&buf, &len))
    {
      buf = NULL;
    }
  }

  if (buf && len > 0)
  {
    Py_BEGIN_ALLOW_THREADS

    v8::internal::Deserializer deserializer((const v8::internal::byte *) buf, len);

    deserializer.GetFlags();

    v8::internal::V8::Initialize(&deserializer);

    Py_END_ALLOW_THREADS 
  }
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

class CPythonExtension : public v8::Extension
{
  py::object m_callback;

  static v8::Handle<v8::Value> CallStub(const v8::Arguments& args)
  {
    v8::HandleScope handle_scope;
    CPythonGIL python_gil;

    py::object func = *static_cast<py::object *>(v8::External::Unwrap(args.Data()));

    py::object result;

    switch (args.Length())
    {
    case 0: result = func(); break;
    case 1: result = func(CJavascriptObject::Wrap(args[0])); break;
    case 2: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1])); break;
    case 3: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]), 
                          CJavascriptObject::Wrap(args[2])); break;
    case 4: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]), 
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3])); break;
    case 5: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]), 
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4])); break;
    case 6: result = func(CJavascriptObject::Wrap(args[0]), CJavascriptObject::Wrap(args[1]), 
                          CJavascriptObject::Wrap(args[2]), CJavascriptObject::Wrap(args[3]),
                          CJavascriptObject::Wrap(args[4]), CJavascriptObject::Wrap(args[5])); break;
    default:
      return v8::ThrowException(v8::Exception::Error(v8::String::New("too many arguments")));
    }

    return handle_scope.Close(CPythonObject::Wrap(result));
  }
public:
  CPythonExtension(const char *name, const char *source, py::object callback, int dep_count, const char**deps)
    : v8::Extension(name, source, dep_count, deps), m_callback(callback)
  {

  }

  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunction(v8::Handle<v8::String> name)
  {
    v8::HandleScope handle_scope;
    CPythonGIL python_gil;

    py::object func;
    v8::String::Utf8Value func_name(name);
    std::string func_name_str(*func_name, func_name.length());

    try
    {
      if (::PyCallable_Check(m_callback.ptr()))
      {      
        func = m_callback(func_name_str);
      }
      else if (::PyObject_HasAttrString(m_callback.ptr(), *func_name))
      {
        func = m_callback.attr(func_name_str.c_str());
      }
      else
      {
        return v8::Handle<v8::FunctionTemplate>();
      }
    }
    catch (const std::exception& ex) { v8::ThrowException(v8::Exception::Error(v8::String::New(ex.what()))); } 
    catch (const py::error_already_set&) { CPythonObject::ThrowIf(); } 
    catch (...) { v8::ThrowException(v8::Exception::Error(v8::String::New("unknown exception"))); } 
    
    v8::Handle<v8::External> func_data = v8::External::New(new py::object(func));
    v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(CallStub, func_data);

    return handle_scope.Close(func_tmpl);
  }
};

CExtension::CExtension(const std::string& name, const std::string& source, 
                       py::object callback, py::list deps, bool autoRegister)
  : m_name(name), m_source(source), m_deps(deps), m_registered(false)
{
  for (Py_ssize_t i=0; i<PyList_Size(deps.ptr()); i++)
  {
    py::extract<const std::string> extractor(::PyList_GetItem(deps.ptr(), i));

    if (extractor.check()) 
    {
      m_depNames.push_back(extractor());
      m_depPtrs.push_back(m_depNames.rbegin()->c_str());
    }
  }

  m_extension.reset(new CPythonExtension(m_name.c_str(), m_source.c_str(), 
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));
  
  if (autoRegister) this->Register();
}