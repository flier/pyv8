#pragma once

#include <stdexcept>
#include <string>

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"

class CEngineException : public CPythonException
{
  const std::string generate(v8::TryCatch& try_catch)
  {
    v8::String::AsciiValue exception(try_catch.Exception());

    return *exception;
  }
public:
  CEngineException(v8::TryCatch& try_catch) 
    : CPythonException(generate(try_catch))
  {
  }

  CEngineException(const std::string& msg)
    : CPythonException(msg)
  {
  }
};

class CScript;

class CEngine
{  
  v8::Persistent<v8::Context> m_context;
  CPythonWrapper m_wrapper;
protected:
  static void ReportFatalError(const char* location, const char* message);
  static void ReportMessage(v8::Handle<v8::Message> message, v8::Handle<v8::Value> data);  
public:
  CEngine(py::object global = py::object());

  ~CEngine()
  {
    m_wrapper.Detach();
    m_context.Dispose();
  }

  boost::shared_ptr<CScript> Compile(const std::string& src);
  py::object Execute(const std::string& src);

  void RaiseError(v8::TryCatch& try_catch);
public:
  static void Expose(void);

  static const std::string GetVersion(void) { return v8::V8::GetVersion(); }

  v8::Persistent<v8::Context>& GetContext(void) { return m_context; }

  py::object ExecuteScript(v8::Persistent<v8::Script>& m_script);
};

class CScript
{
  CEngine& m_engine;
  const std::string m_source;

  v8::Persistent<v8::Script> m_script;  
public:
  CScript(CEngine& engine, const std::string& source, const v8::Persistent<v8::Script>& script) 
    : m_engine(engine), m_source(source), m_script(script)
  {

  }
  ~CScript()
  {
    m_script.Dispose();
  }

  static void Expose(void);

  const std::string GetSource(void) const { return m_source; }

  py::object Run(void) { return m_engine.ExecuteScript(m_script); }
};