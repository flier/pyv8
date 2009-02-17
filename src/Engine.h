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

class CContext : public CJavascriptObject
{
  v8::Persistent<v8::Context> m_context;

  static v8::Handle<v8::Object> GetObject(v8::Handle<v8::Context> context)
  {
    v8::HandleScope handle_scope;

    v8::Context::Scope context_scope(context); 

    v8::Handle<v8::String> proto = v8::String::New("__proto__");

    if (context->Global()->Has(proto))
      return handle_scope.Close(context->Global()->Get(proto)->ToObject());
    else
      return handle_scope.Close(context->Global());
  }
public:
  CContext(v8::Handle<v8::Context> context) 
    : CJavascriptObject(context, GetObject(context))
  {
    v8::HandleScope handle_scope;

    m_context = v8::Persistent<v8::Context>::New(context);
  }

  ~CContext(void)
  {
    //m_context.Dispose();
  }

  void Enter(void) { m_context->Enter(); }
  void Leave(void) { m_context->Exit(); }
  void LeaveWith(py::object exc_type, py::object exc_value, py::object traceback) { return m_context->Exit(); }

  static CContext GetEntered(void) { return CContext(v8::Context::GetEntered()); }
  static CContext GetCurrent(void) { return CContext(v8::Context::GetCurrent()); }
  static bool InContext(void) { return v8::Context::InContext(); }
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

  CContext GetContext(void) { v8::HandleScope handle_scope; return CContext(m_context); }

  boost::shared_ptr<CScript> Compile(const std::string& src);
  py::object Execute(const std::string& src);

  void RaiseError(v8::TryCatch& try_catch);
public:
  static void Expose(void);

  static const std::string GetVersion(void) { return v8::V8::GetVersion(); }

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

  const std::string GetSource(void) const { return m_source; }

  py::object Run(void) { return m_engine.ExecuteScript(m_script); }
};
