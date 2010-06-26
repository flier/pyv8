#pragma once

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

#include "Context.h"

class CScript;

typedef boost::shared_ptr<CScript> CScriptPtr;

class CEngine
{  
protected:

#ifdef SUPPORT_SERIALIZE

  typedef std::map<std::string, int> CounterTable;
  static CounterTable m_counters;

  static int *CounterLookup(const char* name);

#endif

  static void ReportFatalError(const char* location, const char* message);
  static void ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data);    
public:
  py::object PreCompile(const std::string& src);

  CScriptPtr Compile(const std::string& src, const std::string name = std::string(),
                     int line = -1, int col = -1, py::object precompiled = py::object());

  CJavascriptObjectPtr Execute(const std::string& src);

  void RaiseError(v8::TryCatch& try_catch);
public:  
  static void Expose(void);

  static const std::string GetVersion(void) { return v8::V8::GetVersion(); }

  py::object ExecuteScript(v8::Handle<v8::Script> script);

  static void SetFlags(const std::string& flags) { v8::V8::SetFlagsFromString(flags.c_str(), flags.size()); }

  static void SetSerializeEnable(bool value);
  static bool IsSerializeEnabled(void);

  static py::object Serialize(void);
  static void Deserialize(py::object snapshot);
};

class CScript
{
  CEngine& m_engine;

  v8::Persistent<v8::String> m_source;
  v8::Persistent<v8::Script> m_script;  
public:
  CScript(CEngine& engine, v8::Persistent<v8::String> source, v8::Handle<v8::Script> script) 
    : m_engine(engine), m_source(source), m_script(v8::Persistent<v8::Script>::New(script))
  {

  }
  ~CScript()
  {
    m_source.Dispose();
    m_script.Dispose();
  }

#ifdef SUPPORT_AST
  void visit(py::object handler) const;
#endif

  const std::string GetSource(void) const;

  py::object Run(void);
};

#ifdef SUPPORT_EXTENSION

class CExtension 
{
  py::list m_deps;
  std::vector<std::string> m_depNames;
  std::vector<const char *> m_depPtrs;

  bool m_registered;

  boost::shared_ptr<v8::Extension> m_extension;

  static std::vector< boost::shared_ptr<v8::Extension> > s_extensions;
public:
  CExtension(const std::string& name, const std::string& source, py::object callback, py::list dependencies, bool autoRegister);

  const std::string GetName(void) { return m_extension->name(); }
  const std::string GetSource(void) { return m_extension->source(); }

  bool IsRegistered(void) { return m_registered; }
  void Register(void);

  bool IsAutoEnable(void) { return m_extension->auto_enable(); }
  void SetAutoEnable(bool value) { m_extension->set_auto_enable(value); }

  py::list GetDependencies(void) { return m_deps; }

  static py::list GetExtensions(void);
};

#endif // SUPPORT_EXTENSION

#ifdef SUPPORT_PROFILER

class CProfiler
{
public:
  static void Start(void);
  static void Stop(void);

  static bool IsStarted(void);

  static py::tuple GetLogLines(int pos);
};

#endif // SUPPORT_PROFILER
