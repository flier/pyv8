#pragma once

#include <cassert>

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CContext;
class CIsolate;

typedef boost::shared_ptr<CContext> CContextPtr;
typedef boost::shared_ptr<CIsolate> CIsolatePtr;

class CIsolate
{
  logger_t m_logger;
  v8::Isolate *m_isolate;
  bool m_owner;

  void initLogger() {
    m_logger.add_attribute(ISOLATE_ATTR, attrs::constant< const v8::Isolate * >(m_isolate));
  }
public:
  CIsolate(bool owner=false) : m_owner(owner) {
    v8::Isolate::CreateParams params;

    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    m_isolate = v8::Isolate::New(params);

    initLogger();

    BOOST_LOG_SEV(m_logger, trace) << "isolated created";
  }
  CIsolate(v8::Isolate *isolate) : m_isolate(isolate), m_owner(false) {
    initLogger();

    BOOST_LOG_SEV(m_logger, trace) << "isolated wrapped";
  }
  ~CIsolate(void) {
    if (m_owner) {
      ClearDataSlots();

      m_isolate->Dispose();

      BOOST_LOG_SEV(m_logger, trace) << "isolated destroyed";
    }
  }

  v8::Isolate *GetIsolate(void) { return m_isolate; }

  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
    v8::StackTrace::StackTraceOptions options = v8::StackTrace::kOverview) {
    return CJavascriptStackTrace::GetCurrentStackTrace(m_isolate, frame_limit, options);
  }

  static CIsolate Current(void) { return CIsolate(v8::Isolate::GetCurrent()); }

  static py::object GetCurrent(void);

  void Enter(void) {
    m_isolate->Enter();

    BOOST_LOG_SEV(m_logger, trace) << "isolated entered";
  }
  void Leave(void) {
    m_isolate->Exit();

    BOOST_LOG_SEV(m_logger, trace) << "isolated exited";
  }
  void Dispose(void) {
    m_isolate->Dispose();

    BOOST_LOG_SEV(m_logger, trace) << "isolated disposed";
  }

  bool IsLocked(void) {
    bool locked = v8::Locker::IsLocked(m_isolate);

    BOOST_LOG_SEV(m_logger, trace) << "isolated was locked";

    return locked;
  }

  enum Slot {
    ObjectTemplate
  };

  template <typename T>
  v8::Persistent<T> * GetData(Slot slot) {
    return static_cast<v8::Persistent<T> *>(m_isolate->GetData(slot));
  }

  template <typename T>
  void SetData(Slot slot, v8::Persistent<T> * data) {
    m_isolate->SetData(slot, data);
  }

  void ClearDataSlots() {
    delete GetData<v8::ObjectTemplate>(CIsolate::Slot::ObjectTemplate);
  }

  v8::Local<v8::ObjectTemplate> GetObjectTemplate(void) {
    auto object_template = GetData<v8::ObjectTemplate>(CIsolate::Slot::ObjectTemplate);

    if (!object_template) {
      object_template = new v8::Persistent<v8::ObjectTemplate>(m_isolate, CPythonObject::CreateObjectTemplate(m_isolate));

      SetData(CIsolate::Slot::ObjectTemplate, object_template);
    }

    return object_template->Get(m_isolate);
  }
};

class CContext
{
  logger_t m_logger;
  v8::Persistent<v8::Context> m_context;
  py::object m_global;

  void initLogger() {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Context> context = m_context.Get(isolate);

    m_logger.add_attribute(ISOLATE_ATTR, attrs::constant< const v8::Isolate * >(context->GetIsolate()));
    m_logger.add_attribute(CONTEXT_ATTR, attrs::constant< const v8::Context * >(*context));
  }
public:
  CContext(v8::Handle<v8::Context> context);
  CContext(const CContext& context);
  CContext(py::object global, py::list extensions);

  ~CContext()
  {
    BOOST_LOG_SEV(m_logger, trace) << "context destroyed";

    m_context.Reset();
  }

  v8::Handle<v8::Context> Context(void) const { return m_context.Get(v8::Isolate::GetCurrent()); }

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
