#pragma once

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CIsolate final
{
  logger_t m_logger;
  v8::Isolate *m_isolate;
  bool m_owned;

  void initLogger() {
    m_logger.add_attribute(ISOLATE_ATTR, attrs::constant< const v8::Isolate * >(m_isolate));
  }
public:
  CIsolate(bool owned=false);
  CIsolate(v8::Isolate *isolate);
  ~CIsolate(void);

  v8::Isolate *GetIsolate(void) { return m_isolate; }

  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
    v8::StackTrace::StackTraceOptions options = v8::StackTrace::kOverview) {
    return CJavascriptStackTrace::GetCurrentStackTrace(m_isolate, frame_limit, options);
  }

  static CIsolate Current(void) { return CIsolate(v8::Isolate::GetCurrent()); }

  static py::object GetCurrent(void);

  inline void Enter(void) {
    m_isolate->Enter();

    BOOST_LOG_SEV(m_logger, trace) << "isolated entered";
  }
  inline void Leave(void) {
    m_isolate->Exit();

    BOOST_LOG_SEV(m_logger, trace) << "isolated exited";
  }
  inline void Dispose(void) {
    m_isolate->Dispose();

    BOOST_LOG_SEV(m_logger, trace) << "isolated disposed";
  }

  inline bool IsLocked(void) const { return v8::Locker::IsLocked(m_isolate); }

  inline bool InUse(void) const { return m_isolate->IsInUse(); }

  enum Slots {
    ObjectTemplate
  };

  template <typename T>
  inline v8::Persistent<T> * GetData(Slots slot) const {
    return static_cast<v8::Persistent<T> *>(m_isolate->GetData(slot));
  }

  template <typename T>
  inline void SetData(Slots slot, v8::Persistent<T> * data) const {
    m_isolate->SetData(slot, data);
  }

  void ClearDataSlots() const {
    delete GetData<v8::ObjectTemplate>(CIsolate::Slots::ObjectTemplate);
  }

  v8::Local<v8::ObjectTemplate> GetObjectTemplate(void);

  static void Expose(void);
};

typedef boost::shared_ptr<CIsolate> CIsolatePtr;
