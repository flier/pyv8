#pragma once

#include <string>

#include <boost/shared_ptr.hpp>

#include "Context.h"

class CScript;

typedef boost::shared_ptr<CScript> CScriptPtr;

class CEngine
{  
protected:
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
};

class CScript
{
  CEngine& m_engine;
  const std::string m_source;

  v8::Persistent<v8::Script> m_script;  
public:
  CScript(CEngine& engine, const std::string& source, v8::Handle<v8::Script> script) 
    : m_engine(engine), m_source(source), 
      m_script(v8::Persistent<v8::Script>::New(script))
  {

  }
  ~CScript()
  {
    m_script.Dispose();
  }

  const std::string GetSource(void) const { return m_source; }

  py::object Run(void);
};
