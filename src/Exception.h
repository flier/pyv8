#pragma once

#include <cassert>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "Config.h"
#include "Utils.h"

#define BEGIN_HANDLE_PYTHON_EXCEPTION try
#define END_HANDLE_PYTHON_EXCEPTION \
  catch (const std::exception& ex) { v8::Isolate::GetCurrent()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), ex.what()))); } \
  catch (const py::error_already_set&) { CPythonObject::ThrowIf(v8::Isolate::GetCurrent()); } \
  catch (...) { v8::Isolate::GetCurrent()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "unknown exception"))); }

#define BEGIN_HANDLE_JAVASCRIPT_EXCEPTION v8::TryCatch try_catch;
#define END_HANDLE_JAVASCRIPT_EXCEPTION if (try_catch.HasCaught()) CJavascriptException::ThrowIf(try_catch);

class CJavascriptException;

struct ExceptionTranslator
{
  static void Translate(CJavascriptException const& ex);

  static void *Convertible(PyObject* obj);
  static void Construct(PyObject* obj, py::converter::rvalue_from_python_stage1_data* data);
};

class CJavascriptStackTrace;
class CJavascriptStackFrame;

typedef boost::shared_ptr<CJavascriptStackTrace> CJavascriptStackTracePtr;
typedef boost::shared_ptr<CJavascriptStackFrame> CJavascriptStackFramePtr;

class CJavascriptStackTrace
{
  v8::Persistent<v8::StackTrace> m_st;
public:
  CJavascriptStackTrace(v8::Handle<v8::StackTrace> st)
    : m_st(v8::Isolate::GetCurrent(), st)
  {

  }

  CJavascriptStackTrace(const CJavascriptStackTrace& st)
  {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    m_st.Reset(v8::Isolate::GetCurrent(), st.Handle());
  }

  v8::Handle<v8::StackTrace> Handle() const { return v8::Local<v8::StackTrace>::New(v8::Isolate::GetCurrent(), m_st); }

  int GetFrameCount() const { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->GetFrameCount(); }
  CJavascriptStackFramePtr GetFrame(size_t idx) const;

  static CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit,
    v8::StackTrace::StackTraceOptions options = v8::StackTrace::kOverview);

  void Dump(std::ostream& os) const;

  class FrameIterator
    : public boost::iterator_facade<FrameIterator, CJavascriptStackFramePtr const, boost::forward_traversal_tag, CJavascriptStackFramePtr>
  {
    CJavascriptStackTrace *m_st;
    size_t m_idx;
  public:
    FrameIterator(CJavascriptStackTrace *st, size_t idx)
      : m_st(st), m_idx(idx)
    {
    }

    void increment() { m_idx++; }

    bool equal(FrameIterator const& other) const { return m_st == other.m_st && m_idx == other.m_idx; }

    reference dereference() const { return m_st->GetFrame(m_idx); }
  };

  FrameIterator begin(void) { return FrameIterator(this, 0);}
  FrameIterator end(void) { return FrameIterator(this, GetFrameCount());}
};

class CJavascriptStackFrame
{
  v8::Persistent<v8::StackFrame> m_frame;
public:
  CJavascriptStackFrame(v8::Handle<v8::StackFrame> frame)
    : m_frame(v8::Isolate::GetCurrent(), frame)
  {

  }

  CJavascriptStackFrame(const CJavascriptStackFrame& frame)
  {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    m_frame.Reset(v8::Isolate::GetCurrent(), frame.Handle());
  }

  v8::Handle<v8::StackFrame> Handle() const { return v8::Local<v8::StackFrame>::New(v8::Isolate::GetCurrent(), m_frame); }

  int GetLineNumber() const { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->GetLineNumber(); }
  int GetColumn() const { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->GetColumn(); }
  const std::string GetScriptName() const;
  const std::string GetFunctionName() const;
  bool IsEval() const { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->IsEval(); }
  bool IsConstructor() const { v8::HandleScope handle_scope(v8::Isolate::GetCurrent()); return Handle()->IsConstructor(); }
};

class CJavascriptException : public std::runtime_error
{
  PyObject *m_type;

  v8::Persistent<v8::Value> m_exc, m_stack;
  v8::Persistent<v8::Message> m_msg;

  friend struct ExceptionTranslator;

  static const std::string Extract(v8::TryCatch& try_catch);
protected:
  CJavascriptException(v8::TryCatch& try_catch, PyObject *type)
    : std::runtime_error(Extract(try_catch)), m_type(type)
  {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    m_exc.Reset(v8::Isolate::GetCurrent(), try_catch.Exception());
    m_stack.Reset(v8::Isolate::GetCurrent(), try_catch.StackTrace());
    m_msg.Reset(v8::Isolate::GetCurrent(), try_catch.Message());
  }
public:
  CJavascriptException(const std::string& msg, PyObject *type = NULL)
    : std::runtime_error(msg), m_type(type)
  {
  }

  CJavascriptException(const CJavascriptException& ex)
    : std::runtime_error(ex.what()), m_type(ex.m_type)
  {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

    m_exc.Reset(v8::Isolate::GetCurrent(), ex.Exception());
    m_stack.Reset(v8::Isolate::GetCurrent(), ex.Stack());
    m_msg.Reset(v8::Isolate::GetCurrent(), ex.Message());
  }

  ~CJavascriptException() throw()
  {
    if (!m_exc.IsEmpty()) m_exc.Reset();
    if (!m_msg.IsEmpty()) m_msg.Reset();
  }

  v8::Handle<v8::Value> Exception() const { return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), m_exc); }
  v8::Handle<v8::Value> Stack() const { return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), m_stack); }
  v8::Handle<v8::Message> Message() const { return v8::Local<v8::Message>::New(v8::Isolate::GetCurrent(), m_msg); }

  const std::string GetName(void);
  const std::string GetMessage(void);
  const std::string GetScriptName(void);
  int GetLineNumber(void);
  int GetStartPosition(void);
  int GetEndPosition(void);
  int GetStartColumn(void);
  int GetEndColumn(void);
  const std::string GetSourceLine(void);
  const std::string GetStackTrace(void);

  void PrintCallStack(py::object file);

  static void ThrowIf(v8::TryCatch& try_catch);

  static void Expose(void);
};
