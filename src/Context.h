#pragma once

#include <cassert>

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"

class CContext;

typedef boost::shared_ptr<CContext> CContextPtr;

class CContext : public CJavascriptObject
{
  v8::Persistent<v8::Context> m_context;
public:
  CContext(v8::Handle<v8::Context> context) 
    : CJavascriptObject(context, context->Global())
  {
    v8::HandleScope handle_scope;

    m_context = v8::Persistent<v8::Context>::New(context);
  }

  ~CContext()
  {
    assert(!m_context.IsEmpty());

    m_context.Dispose();
  }  

  v8::Handle<v8::Context> Handle() { return m_context; }

  void Enter(void) { m_context->Enter(); }
  void Leave(void) { m_context->Exit(); }
  void LeaveWith(py::object exc_type, py::object exc_value, py::object traceback) { return m_context->Exit(); }

  static CContextPtr GetEntered(void) { return CContextPtr(new CContext(v8::Context::GetEntered())); }
  static CContextPtr GetCurrent(void) { return CContextPtr(new CContext(v8::Context::GetCurrent())); }
  static bool InContext(void) { return v8::Context::InContext(); }

  static CContextPtr Create(py::object global = py::object());

  static CPythonWrapper *GetWrapper(v8::Handle<v8::Context> context);

  static void Expose(void);
};
