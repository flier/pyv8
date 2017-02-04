#pragma once

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CIsolate
{
  logger_t m_logger;
  v8::Isolate *m_isolate;
  bool m_owner;

  void initLogger() {
    m_logger.add_attribute(ISOLATE_ATTR, attrs::constant< const v8::Isolate * >(m_isolate));
  }
public:
  CIsolate(bool owner=false);
  CIsolate(v8::Isolate *isolate);
  ~CIsolate(void);

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

  bool InUse(void) { return m_isolate->IsInUse(); }

  enum Slots {
    ObjectTemplate
  };

  template <typename T>
  v8::Persistent<T> * GetData(Slots slot) {
    return static_cast<v8::Persistent<T> *>(m_isolate->GetData(slot));
  }

  template <typename T>
  void SetData(Slots slot, v8::Persistent<T> * data) {
    m_isolate->SetData(slot, data);
  }

  void ClearDataSlots() {
    delete GetData<v8::ObjectTemplate>(CIsolate::Slots::ObjectTemplate);
  }

  v8::Local<v8::ObjectTemplate> GetObjectTemplate(void);

  static void Expose(void);
};

typedef boost::shared_ptr<CIsolate> CIsolatePtr;
