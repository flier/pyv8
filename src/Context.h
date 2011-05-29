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
  v8::Isolate *m_isolate;
  bool m_owner;
public:
  CIsolate(bool owner=false) : m_owner(owner) { m_isolate = v8::Isolate::New(); }
  CIsolate(v8::Isolate *isolate) : m_isolate(isolate), m_owner(false) {}
  ~CIsolate(void) { if (m_owner) m_isolate->Dispose(); }

  v8::Isolate *GetIsolate(void) { return m_isolate; }

  static py::object GetCurrent(void);

  void Enter(void) { m_isolate->Enter(); }
  void Leave(void) { m_isolate->Exit(); }
  void Dispose(void) { m_isolate->Dispose(); }

  bool IsLocked(void) { return v8::Locker::IsLocked(m_isolate); }
};

class CContext 
{
  v8::Persistent<v8::Context> m_context;
public:
  CContext(v8::Handle<v8::Context> context);
  CContext(const CContext& context);
  CContext(py::object global, py::list extensions);

  ~CContext()
  {
    m_context.Dispose();
  }  

  v8::Handle<v8::Context> Handle(void) { return m_context; }

  py::object GetGlobal(void);

  py::str GetSecurityToken(void);
  void SetSecurityToken(py::str token);

  bool IsEntered(void) { return !m_context.IsEmpty(); }
  void Enter(void) { m_context->Enter(); }
  void Leave(void) { m_context->Exit(); }

  py::object Evaluate(const std::string& src, const std::string name = std::string(),
                      int line = -1, int col = -1, py::object precompiled = py::object());
  py::object EvaluateW(const std::wstring& src, const std::wstring name = std::wstring(),
                       int line = -1, int col = -1, py::object precompiled = py::object());

  static py::object GetEntered(void);
  static py::object GetCurrent(void);
  static py::object GetCalling(void);
  static bool InContext(void) { return v8::Context::InContext(); }

  static void Expose(void);
};
