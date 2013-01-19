#include "Engine.h"

#include <boost/preprocessor.hpp>

#include "V8Internal.h"

#ifdef SUPPORT_SERIALIZE
  CEngine::CounterTable CEngine::m_counters;
#endif

#ifdef SUPPORT_AST
  #include "AST.h"
#endif

struct MemoryAllocationCallbackBase
{
  virtual void Set(py::object callback) = 0;
};

template <v8::ObjectSpace SPACE, v8::AllocationAction ACTION>
struct MemoryAllocationCallbackStub : public MemoryAllocationCallbackBase
{  
  static py::object s_callback;

  static void onMemoryAllocation(v8::ObjectSpace space, v8::AllocationAction action, int size)
  {
    if (s_callback.ptr() != Py_None) s_callback(space, action, size);
  }

  virtual void Set(py::object callback)
  {
    if (s_callback.ptr() == Py_None && callback.ptr() != Py_None)
    {
      v8::V8::AddMemoryAllocationCallback(&onMemoryAllocation, SPACE, ACTION);  
    }
    else if (s_callback.ptr() != Py_None && callback.ptr() == Py_None)
    {
      v8::V8::RemoveMemoryAllocationCallback(&onMemoryAllocation);
    }

    s_callback = callback;
  }
};

template<v8::ObjectSpace space, v8::AllocationAction action> 
py::object MemoryAllocationCallbackStub<space, action>::s_callback;

class MemoryAllocationManager
{
  typedef std::map<std::pair<v8::ObjectSpace, v8::AllocationAction>, MemoryAllocationCallbackBase *> CallbackMap;

  static CallbackMap s_callbacks;
public:
  static void Init(void)
  {
#define ADD_CALLBACK_STUB(space, action) s_callbacks[std::make_pair(space, action)] = new MemoryAllocationCallbackStub<space, action>()

#define OBJECT_SPACES (kObjectSpaceNewSpace) (kObjectSpaceOldPointerSpace) (kObjectSpaceOldDataSpace) \
(kObjectSpaceCodeSpace) (kObjectSpaceMapSpace) (kObjectSpaceLoSpace) (kObjectSpaceAll)
#define ALLOCATION_ACTIONS (kAllocationActionAllocate) (kAllocationActionFree) (kAllocationActionAll)

#define ADD_CALLBACK_STUBS(r, action, space) ADD_CALLBACK_STUB(v8::space, v8::action);

    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionAllocate, OBJECT_SPACES);
    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionFree, OBJECT_SPACES);
    BOOST_PP_SEQ_FOR_EACH(ADD_CALLBACK_STUBS, kAllocationActionAll, OBJECT_SPACES);
  }

  static void SetCallback(py::object callback, v8::ObjectSpace space, v8::AllocationAction action)
  {
    s_callbacks[std::make_pair(space, action)]->Set(callback);
  }
};

MemoryAllocationManager::CallbackMap MemoryAllocationManager::s_callbacks;
  
void CEngine::Expose(void)
{
#ifndef SUPPORT_SERIALIZE
  v8::V8::Initialize();
  v8::V8::SetFatalErrorHandler(ReportFatalError);
  v8::V8::AddMessageListener(ReportMessage);
#endif

  MemoryAllocationManager::Init();

  void (*terminateExecution)(int) = &v8::V8::TerminateExecution;

  py::enum_<v8::ObjectSpace>("JSObjectSpace")
    .value("New", v8::kObjectSpaceNewSpace)
  
    .value("OldPointer", v8::kObjectSpaceOldPointerSpace)
    .value("OldData", v8::kObjectSpaceOldDataSpace)
    .value("Code", v8::kObjectSpaceCodeSpace)
    .value("Map", v8::kObjectSpaceMapSpace)
    .value("Lo", v8::kObjectSpaceLoSpace)

    .value("All", v8::kObjectSpaceAll);

  py::enum_<v8::AllocationAction>("JSAllocationAction")
    .value("alloc", v8::kAllocationActionAllocate)
    .value("free", v8::kAllocationActionFree)
    .value("all", v8::kAllocationActionAll);

  py::enum_<CScript::LanguageMode>("JSLanguageMode")
    .value("CLASSIC", CScript::CLASSIC_MODE)
    .value("STRICT", CScript::STRICT_MODE)
    .value("EXTENDED", CScript::EXTENDED_MODE);
  
  py::class_<CEngine, boost::noncopyable>("JSEngine", "JSEngine is a backend Javascript engine.")
    .def(py::init<>("Create a new script engine instance."))
    .add_static_property("version", &CEngine::GetVersion, 
                         "Get the V8 engine version.")

    .add_static_property("dead", &v8::V8::IsDead,
                         "Check if V8 is dead and therefore unusable.")

    .add_static_property("currentThreadId", &v8::V8::GetCurrentThreadId,
                         "The V8 thread id of the calling thread.")

    .def("setFlags", &CEngine::SetFlags, "Sets V8 flags from a string.")
    .staticmethod("setFlags")

    .def("collect", &CEngine::CollectAllGarbage, (py::arg("force")=true),
         "Performs a full garbage collection. Force compaction if the parameter is true.")
    .staticmethod("collect")

  #ifdef SUPPORT_SERIALIZE
    .add_static_property("serializeEnabled", &CEngine::IsSerializeEnabled, &CEngine::SetSerializeEnable)

    .def("serialize", &CEngine::Serialize)
    .staticmethod("serialize")

    .def("deserialize", &CEngine::Deserialize)
    .staticmethod("deserialize")
  #endif

    .def("terminateThread", terminateExecution, (py::arg("thread_id")),
         "Forcefully terminate execution of a JavaScript thread.")
    .staticmethod("terminateThread")

    .def("terminateAllThreads", &CEngine::TerminateAllThreads,
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

    .def("setMemoryLimit", &CEngine::SetMemoryLimit, (py::arg("max_young_space_size") = 0,
                                                py::arg("max_old_space_size") = 0,
                                                py::arg("max_executable_size") = 0),
         "Specifies the limits of the runtime's memory use."
         "You must set the heap size before initializing the VM"
         "the size cannot be adjusted after the VM is initialized.")
    .staticmethod("setMemoryLimit")

    .def("setMemoryAllocationCallback", &MemoryAllocationManager::SetCallback, 
                                        (py::arg("callback"),
                                         py::arg("space") = v8::kObjectSpaceAll, 
                                         py::arg("action") = v8::kAllocationActionAll),
                                        "Enables the host application to provide a mechanism to be notified "
                                        "and perform custom logging when V8 Allocates Executable Memory.")
    .staticmethod("setMemoryAllocationCallback")

    .def("precompile", &CEngine::PreCompile, (py::arg("source")))
    .def("precompile", &CEngine::PreCompileW, (py::arg("source")))

    .def("compile", &CEngine::Compile, (py::arg("source"), 
                                        py::arg("name") = std::string(),
                                        py::arg("line") = -1,
                                        py::arg("col") = -1,
                                        py::arg("precompiled") = py::object()))    
    .def("compile", &CEngine::CompileW, (py::arg("source"), 
                                         py::arg("name") = std::wstring(),
                                         py::arg("line") = -1,
                                         py::arg("col") = -1,
                                         py::arg("precompiled") = py::object()))    
    ;

  py::class_<CScript, boost::noncopyable>("JSScript", "JSScript is a compiled JavaScript script.", py::no_init)
    .add_property("source", &CScript::GetSource, "the source code")

    .def("run", &CScript::Run, "Execute the compiled code.")

  #ifdef SUPPORT_AST
    .def("visit", &CScript::visit, (py::arg("handler"),
                                    py::arg("mode") = CScript::CLASSIC_MODE), 
         "Visit the AST of code with the callback handler.")
  #endif
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CScript>, 
    py::objects::make_ptr_instance<CScript, 
    py::objects::pointer_holder<boost::shared_ptr<CScript>, CScript> > >();

#ifdef SUPPORT_EXTENSION

  py::class_<CExtension, boost::noncopyable>("JSExtension", "JSExtension is a reusable script module.", py::no_init)    
    .def(py::init<const std::string&, const std::string&, py::object, py::list, bool>((py::arg("name"), 
                                                                                       py::arg("source"),
                                                                                       py::arg("callback") = py::object(),
                                                                                       py::arg("dependencies") = py::list(),
                                                                                       py::arg("register") = true)))
    .def(py::init<const std::wstring&, const std::wstring&, py::object, py::list, bool>((py::arg("name"), 
                                                                                         py::arg("source"),
                                                                                         py::arg("callback") = py::object(),
                                                                                         py::arg("dependencies") = py::list(),
                                                                                         py::arg("register") = true)))
    .add_static_property("extensions", &CExtension::GetExtensions)

    .add_property("name", &CExtension::GetName, "The name of extension")
    .add_property("source", &CExtension::GetSource, "The source code of extension")
    .add_property("dependencies", &CExtension::GetDependencies, "The extension dependencies which will be load before this extension")

    .add_property("autoEnable", &CExtension::IsAutoEnable, &CExtension::SetAutoEnable, "Enable the extension by default.")

    .add_property("registered", &CExtension::IsRegistered, "The extension has been registerd")
    .def("register", &CExtension::Register, "Register the extension")
    ;

#endif 
  
  py::class_<CProfiler, boost::noncopyable>("JSProfiler", py::init<>())
    .add_static_property("started", &CProfiler::IsStarted)
    .def("start", &CProfiler::Start)  
    .staticmethod("start")
    .def("stop", &CProfiler::Stop)    
    .staticmethod("stop")

    .add_static_property("paused", &v8::V8::IsProfilerPaused)
    .def("pause", &v8::V8::PauseProfiler)
    .staticmethod("pause")
    .def("resume", &v8::V8::ResumeProfiler)
    .staticmethod("resume")
    ;
}

#ifdef SUPPORT_SERIALIZE

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
    v8i::Serializer::Enable();
  else
    v8i::Serializer::Disable();
}
bool CEngine::IsSerializeEnabled(void)
{
  return v8i::Serializer::enabled();
}

struct PyBufferByteSink : public v8i::SnapshotByteSink
{
  std::vector<v8i::byte> m_data;

  virtual void Put(int byte, const char* description) 
  {
    m_data.push_back(byte);
  }
};

py::object CEngine::Serialize(void)
{
  v8::V8::Initialize();

  v8i::StatsTable::SetCounterFunction(&CEngine::CounterLookup);

  PyBufferByteSink sink;

  v8i::Serializer serializer(&sink);

  v8i::Heap::CollectAllGarbage(true);

  serializer.Serialize();

  py::object obj(py::handle<>(::PyBuffer_New(sink.m_data.size())));

  void *buf = NULL;
  Py_ssize_t len = 0;

  if (0 == ::PyObject_AsWriteBuffer(obj.ptr(), &buf, &len) && buf && len > 0)
  {
    memcpy(buf, &sink.m_data[0], len);    
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

    v8i::SnapshotByteSource source((const v8i::byte *) buf, len);
    v8i::Deserializer deserializer(&source);

    //deserializer.Deserialize();

    v8i::V8::Initialize(&deserializer);

    Py_END_ALLOW_THREADS 
  }
}

#endif // SUPPORT_SERIALIZE

void CEngine::CollectAllGarbage(bool force_compaction)
{
  HEAP->CollectAllGarbage(force_compaction);
}

void CEngine::TerminateAllThreads(void)
{
  v8::V8::TerminateExecution();
}

void CEngine::ReportFatalError(const char* location, const char* message)
{
  std::ostringstream oss;

  oss << "<" << location << "> " << message;

  throw CJavascriptException(oss.str());
}

void CEngine::ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data)
{
  v8::String::Utf8Value filename(message->GetScriptResourceName());
  int lineno = message->GetLineNumber();
  v8::String::Utf8Value sourceline(message->GetSourceLine());

  std::ostringstream oss;

  oss << *filename << ":" << lineno << " -> " << *sourceline;

  throw CJavascriptException(oss.str());
}

bool CEngine::SetMemoryLimit(int max_young_space_size, int max_old_space_size, int max_executable_size)
{
  v8::ResourceConstraints limit;

  limit.set_max_young_space_size(max_young_space_size);
  limit.set_max_old_space_size(max_old_space_size);
  limit.set_max_executable_size(max_executable_size);

  return v8::SetResourceConstraints(&limit);
}

py::object CEngine::InternalPreCompile(v8::Handle<v8::String> src)
{
  v8::TryCatch try_catch;

  std::auto_ptr<v8::ScriptData> precompiled;

  Py_BEGIN_ALLOW_THREADS

  precompiled.reset(v8::ScriptData::PreCompile(src));

  Py_END_ALLOW_THREADS 

  if (!precompiled.get()) CJavascriptException::ThrowIf(try_catch);
  if (precompiled->HasError()) throw CJavascriptException("fail to compile", ::PyExc_SyntaxError);

  py::object obj(py::handle<>(::PyBuffer_New(precompiled->Length())));

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

boost::shared_ptr<CScript> CEngine::InternalCompile(v8::Handle<v8::String> src, 
                                                    v8::Handle<v8::Value> name,
                                                    int line, int col,
                                                    py::object precompiled)
{
  if (!v8::Context::InContext())
  {
    throw CJavascriptException("please enter a context first");
  }

  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Persistent<v8::String> script_source = v8::Persistent<v8::String>::New(src);  

  v8::Handle<v8::Script> script;
  std::auto_ptr<v8::ScriptData> script_data;

  if (PyBuffer_Check(precompiled.ptr()))
  {
    const void *buf = NULL;
    Py_ssize_t len = 0;

    if (0 == ::PyObject_AsReadBuffer(precompiled.ptr(), &buf, &len) && buf && len > 0)
    {
      script_data.reset(v8::ScriptData::New((const char *)buf, len));
    }
  }

  Py_BEGIN_ALLOW_THREADS

  if (line >= 0 && col >= 0)
  {
    v8::ScriptOrigin script_origin(name, v8::Integer::New(line), v8::Integer::New(col));

    script = v8::Script::Compile(script_source, &script_origin, script_data.get());
  }
  else
  {
    v8::ScriptOrigin script_origin(name);

    script = v8::Script::Compile(script_source, &script_origin, script_data.get());
  }

  Py_END_ALLOW_THREADS 

  if (script.IsEmpty()) CJavascriptException::ThrowIf(try_catch);

  return boost::shared_ptr<CScript>(new CScript(*this, script_source, script));
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
    if (try_catch.HasCaught()) 
    {
      if(!try_catch.CanContinue() && ::PyErr_Occurred()) 
      {
        throw py::error_already_set();         
      }

      CJavascriptException::ThrowIf(try_catch);
    }

    result = v8::Null();
  }

  return CJavascriptObject::Wrap(result);
}

#ifdef SUPPORT_AST

void CScript::visit(py::object handler, LanguageMode mode) const
{
  v8::HandleScope handle_scope;

  v8i::Handle<v8i::Object> obj = v8::Utils::OpenHandle(*m_script);
  v8i::Handle<v8i::SharedFunctionInfo> func = obj->IsSharedFunctionInfo() ?
    v8i::Handle<v8i::SharedFunctionInfo>(v8i::SharedFunctionInfo::cast(*obj)) :
    v8i::Handle<v8i::SharedFunctionInfo>(v8i::JSFunction::cast(*obj)->shared());
  v8i::Handle<v8i::Script> script(v8i::Script::cast(func->script()));

  v8i::CompilationInfoWithZone info(script);

  info.MarkAsGlobal();

  v8i::Isolate *isolate = info.isolate();
  v8i::ZoneScope zone_scope(info.zone(), v8i::DELETE_ON_EXIT);
  v8i::PostponeInterruptsScope postpone(isolate);
  v8i::FixedArray* array = isolate->native_context()->embedder_data();

  script->set_context_data(array->get(0));
  
  if (v8i::ParserApi::Parse(&info, mode))
  {
    if (::PyObject_HasAttrString(handler.ptr(), "onProgram"))
    {
      handler.attr("onProgram")(CAstFunctionLiteral(info.function()));
    }
  }
}

#endif

const std::string CScript::GetSource(void) const 
{ 
  v8::String::Utf8Value source(m_source); 

  return std::string(*source, source.length());
}

py::object CScript::Run(void) 
{ 
  v8::HandleScope handle_scope;

  return m_engine.ExecuteScript(m_script); 
}

#ifdef SUPPORT_EXTENSION

class CPythonExtension : public v8::Extension
{
  py::object m_callback;  

  static v8::Handle<v8::Value> CallStub(const v8::Arguments& args)
  {
    v8::HandleScope handle_scope;
    CPythonGIL python_gil;
    py::object func = *static_cast<py::object *>(v8::External::Cast(*args.Data())->Value());

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
      return v8::ThrowException(v8::Exception::Error(v8::String::NewSymbol("too many arguments")));
    }

    return handle_scope.Close(CPythonObject::Wrap(result));
  }
public:
  CPythonExtension(const char *name, const char *source, py::object callback, int dep_count, const char**deps)
    : v8::Extension(strdup(name), strdup(source), dep_count, deps), m_callback(callback)
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
    catch (...) { v8::ThrowException(v8::Exception::Error(v8::String::NewSymbol("unknown exception"))); } 
    
    v8::Handle<v8::External> func_data = v8::External::New(new py::object(func));
    v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(CallStub, func_data);

    return handle_scope.Close(func_tmpl);
  }
};

std::vector< boost::shared_ptr<v8::Extension> > CExtension::s_extensions;

CExtension::CExtension(const std::string& name, const std::string& source, 
                       py::object callback, py::list deps, bool autoRegister)
  : m_deps(deps), m_registered(false)
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

  m_extension.reset(new CPythonExtension(name.c_str(), source.c_str(), 
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));
  
  if (autoRegister) this->Register();
}

CExtension::CExtension(const std::wstring& name, const std::wstring& source, 
                       py::object callback, py::list deps, bool autoRegister)
  : m_deps(deps), m_registered(false)
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

  std::string utf8_name = EncodeUtf8(name), utf8_source = EncodeUtf8(source);

  if (!v8i::String::IsAscii(utf8_name.c_str(), utf8_name.size()) ||
      !v8i::String::IsAscii(utf8_source.c_str(), utf8_source.size())) 
  {
    throw CJavascriptException("v8 is not support NON-ASCII name or source in the JS extension");
  }

  m_extension.reset(new CPythonExtension(utf8_name.c_str(), utf8_source.c_str(), 
    callback, m_depPtrs.size(), m_depPtrs.empty() ? NULL : &m_depPtrs[0]));

  if (autoRegister) this->Register();
}

void CExtension::Register(void) 
{ 
  v8::RegisterExtension(m_extension.get()); 
  
  m_registered = true; 
  
  s_extensions.push_back(m_extension); 
}

py::list CExtension::GetExtensions(void)
{
  v8::RegisteredExtension *ext = v8::RegisteredExtension::first_extension(); 
  py::list extensions;

  while (ext)
  {
    extensions.append(ext->extension()->name());

    ext = ext->next();
  }

  return extensions;
}

#endif // SUPPORT_EXTENSION

#ifdef SUPPORT_PROFILER

void CProfiler::Start(void)
{
  char params[] = "--prof --prof_auto --logfile=*";

  v8::V8::SetFlagsFromString(params, strlen(params));
  LOGGER->SetUp();
}

void CProfiler::Stop(void)
{
  LOGGER->TearDown();
}

bool CProfiler::IsStarted(void)
{
  return v8i::RuntimeProfiler::IsEnabled();
}

#endif // SUPPORT_PROFILER
