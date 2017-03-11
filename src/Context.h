#pragma once

#include <functional>

#include <boost/shared_ptr.hpp>

#include "Wrapper.h"
#include "Utils.h"

class CContext final
{
  v8::Persistent<v8::Context> m_context;
  py::object m_global;

private: // Embeded Data
  enum EmbedderDataFields
  {
    DebugIdIndex = v8::Context::kDebugIdIndex,
    LoggerIndex,
    GlobalObjectIndex,
  };

  template <typename T>
  static T *GetEmbedderData(v8::Handle<v8::Context> context, EmbedderDataFields index, std::function<T *()> creator = nullptr)
  {
    assert(!context.IsEmpty());
    assert(index > DebugIdIndex);

    auto value = static_cast<T *>(v8::Handle<v8::External>::Cast(context->GetEmbedderData(index))->Value());

    if (!value && creator)
    {
      value = creator();

      SetEmbedderData(context, index, value);
    }

    return value;
  }

  template <typename T>
  static void SetEmbedderData(v8::Handle<v8::Context> context, EmbedderDataFields index, T *data)
  {
    assert(!context.IsEmpty());
    assert(index > DebugIdIndex);
    assert(data);

    context->SetEmbedderData(index, v8::External::New(context->GetIsolate(), (void *)data));
  }

  static logger_t &GetLogger(v8::Handle<v8::Context> context);

  logger_t &logger(v8::Isolate *isolate = v8::Isolate::GetCurrent())
  {
    v8::HandleScope handle_scope(isolate);

    auto context = m_context.Get(isolate);

    return GetLogger(context);
  }

public:
  CContext(v8::Handle<v8::Context> context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
  CContext(const CContext &context, v8::Isolate *isolate = v8::Isolate::GetCurrent());
  CContext(py::object global, py::list extensions, v8::Isolate *isolate = v8::Isolate::GetCurrent());
  ~CContext() { Dispose(false); }

  void Dispose(bool disposed = true, v8::Isolate *isolate = v8::Isolate::GetCurrent());

  inline v8::Handle<v8::Context> Context(v8::Isolate *isolate = v8::Isolate::GetCurrent()) const { return m_context.Get(isolate); }

  py::object GetGlobal(void) const;

  py::str GetSecurityToken(void) const;
  void SetSecurityToken(py::str token);

  bool IsEntered(void) const { return !m_context.IsEmpty(); }

  void Enter(void);
  void Leave(void);

  py::object Evaluate(const std::string &src, const std::string name = std::string(), int line = -1, int col = -1);
  py::object EvaluateW(const std::wstring &src, const std::string name = std::string(), int line = -1, int col = -1);

  static py::object GetEntered(v8::Isolate *isolate = v8::Isolate::GetCurrent());
  static py::object GetCurrent(v8::Isolate *isolate = v8::Isolate::GetCurrent());
  static py::object GetCalling(v8::Isolate *isolate = v8::Isolate::GetCurrent());

  static bool InContext(v8::Isolate *isolate = v8::Isolate::GetCurrent()) { return isolate->InContext(); }

  static logger_t &Logger(v8::Isolate *isolate = v8::Isolate::GetCurrent())
  {
    v8::HandleScope handle_scope(isolate);

    return GetLogger(isolate->GetCurrentContext());
  }

  static void Expose(void);
};

typedef boost::shared_ptr<CContext> CContextPtr;
