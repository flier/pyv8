#pragma once

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CContext
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

  v8::Handle<v8::Context> Context(v8::Isolate *isolate = v8::Isolate::GetCurrent()) const {
    return m_context.Get(isolate);
  }

  py::object GetGlobal(void);

  py::str GetSecurityToken(void);
  void SetSecurityToken(py::str token);

  bool IsEntered(void) { return !m_context.IsEmpty(); }

  void Enter(void) {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    Context()->Enter();

    BOOST_LOG_SEV(m_logger, trace) << "context entered";
  }
  void Leave(void) {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    Context()->Exit();

    BOOST_LOG_SEV(m_logger, trace) << "context exited";
  }

  py::object Evaluate(const std::string& src, const std::string name = std::string(), int line = -1, int col = -1);
  py::object EvaluateW(const std::wstring& src, const std::string name = std::string(), int line = -1, int col = -1);

  static py::object GetEntered(void);
  static py::object GetCurrent(void);
  static py::object GetCalling(void);

  static bool InContext(void) { return v8::Isolate::GetCurrent()->InContext(); }

  static void Expose(void);
};

typedef boost::shared_ptr<CContext> CContextPtr;

