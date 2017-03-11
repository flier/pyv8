#pragma once

#include <functional>

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CIsolateBase
{
protected:
  v8::Isolate *m_isolate;

  CIsolateBase(v8::Isolate *isolate) : m_isolate(isolate) { assert(isolate); }
  virtual ~CIsolateBase() = default;

public: // Operators
  inline v8::Isolate *operator*(void) { return m_isolate; }
  inline v8::Isolate *operator->(void) { return m_isolate; }

protected: // Data Slots
  enum DataSlots
  {
    LoggerIndex,
    ObjectTemplateIndex
  };

  template <typename T>
  inline T *GetData(DataSlots slot, std::function<T *()> creator = nullptr) const
  {
    auto value = static_cast<T *>(m_isolate->GetData(slot));

    if (value == nullptr && creator != nullptr)
    {
      value = creator();

      m_isolate->SetData(slot, value);
    }

    return value;
  }

  template <typename T>
  inline void SetData(DataSlots slot, T *data) const
  {
    m_isolate->SetData(slot, data);
  }

public: // Internal Properties
  inline v8::Isolate *GetIsolate(void) const { return m_isolate; }

  logger_t &Logger(void);
};

class CIsolateWrapper : public CIsolateBase
{
protected:
  CIsolateWrapper(v8::Isolate *isolate) : CIsolateBase(isolate) {}

public: // Properties
  inline bool IsLocked(void) const { return v8::Locker::IsLocked(m_isolate); }

  inline bool InUse(void) const { return m_isolate->IsInUse(); }

public: // Methods
  void Enter(void)
  {
    BOOST_LOG_SEV(Logger(), trace) << "enter isolate";

    m_isolate->Enter();
  }

  void Leave(void)
  {
    BOOST_LOG_SEV(Logger(), trace) << "exit isolate";

    m_isolate->Exit();
  }

  void Dispose(void)
  {
    BOOST_LOG_SEV(Logger(), trace) << "destroy isolate";

    m_isolate->Dispose(); // delete m_isolate;
  }

public: // Python Helper
  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
                                                v8::StackTrace::StackTraceOptions options = v8::StackTrace::kOverview)
  {
    return CJavascriptStackTrace::GetCurrentStackTrace(m_isolate, frame_limit, options);
  }

  static py::object GetCurrent(void);
};

class CIsolate : public CIsolateWrapper
{
public:
  CIsolate(v8::Isolate *isolate);
  virtual ~CIsolate() = default;

public: // Internal Properties
  static CIsolate Current(void) { return CIsolate(v8::Isolate::GetCurrent()); }

  v8::Local<v8::ObjectTemplate> ObjectTemplate(void);
};

class CManagedIsolate : public CIsolateWrapper, private boost::noncopyable
{
  static v8::Isolate *CreateIsolate();

  void ClearDataSlots() const;

public:
  CManagedIsolate();
  virtual ~CManagedIsolate(void);

  static void Expose(void);
};

typedef boost::shared_ptr<CIsolateWrapper> CIsolateWrapperPtr;