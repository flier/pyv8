#pragma once

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CContext final
{
  logger_t m_logger;
  v8::Persistent<v8::Context> m_context;
  py::object m_global;

  void initLogger(v8::Isolate *isolate = v8::Isolate::GetCurrent()) {
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Context> context = m_context.Get(isolate);

    m_logger.add_attribute(ISOLATE_ATTR, attrs::constant< const v8::Isolate * >(context->GetIsolate()));
    m_logger.add_attribute(CONTEXT_ATTR, attrs::constant< const v8::Context * >(*context));
  }
public:
  CContext(v8::Handle<v8::Context> context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
  CContext(const CContext& context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
  CContext(py::object global, py::list extensions, v8::Isolate *isolate = v8::Isolate::GetCurrent());

  ~CContext()
  {
    BOOST_LOG_SEV(m_logger, trace) << "context destroyed";

    m_context.Reset();
  }

  inline v8::Handle<v8::Context> Context(void) const { return m_context.Get(v8::Isolate::GetCurrent()); }

  py::object GetGlobal(void) const;

  py::str GetSecurityToken(void) const;
  void SetSecurityToken(py::str token);

  bool IsEntered(void) const { return !m_context.IsEmpty(); }

  void Enter(void);
  void Leave(void);

  py::object Evaluate(const std::string& src, const std::string name = std::string(), int line = -1, int col = -1);
  py::object EvaluateW(const std::wstring& src, const std::string name = std::string(), int line = -1, int col = -1);

  static py::object GetEntered(v8::Isolate *isolate = v8::Isolate::GetCurrent());
  static py::object GetCurrent(v8::Isolate *isolate = v8::Isolate::GetCurrent());
  static py::object GetCalling(v8::Isolate *isolate = v8::Isolate::GetCurrent());

  static bool InContext(v8::Isolate *isolate = v8::Isolate::GetCurrent()) { return isolate->InContext(); }

  static void Expose(void);
};

typedef boost::shared_ptr<CContext> CContextPtr;

