#pragma once

#include <cassert>
#include <stdexcept>

#include <v8.h>

#ifdef _WIN32
# pragma warning( push )
# pragma warning( disable : 4100 ) // 'identifier' : unreferenced formal parameter
# pragma warning( disable : 4127 ) // conditional expression is constant
# pragma warning( disable : 4244 ) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
# pragma warning( disable : 4512 ) // 'class' : assignment operator could not be generated
#endif

#include <boost/python.hpp>
namespace py = boost::python;

#ifdef _WIN32
# pragma comment( lib, "v8.lib" )
# pragma comment( lib, "v8_base.lib" )
# pragma comment( lib, "v8_snapshot.lib" )

# pragma warning( pop )
#endif 

struct CPythonGIL
{
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};

class CJavascriptException;

struct ExceptionTranslator
{
  static void Translate(CJavascriptException const& ex);

  static void *Convertible(PyObject* obj);
  static void Construct(PyObject* obj, py::converter::rvalue_from_python_stage1_data* data);
};

class CJavascriptException : public std::runtime_error
{
  PyObject *m_type;

  v8::Persistent<v8::Value> m_exc, m_stack;
  v8::Persistent<v8::Message> m_msg;

  friend struct ExceptionTranslator;

  static const std::string Extract(v8::TryCatch& try_catch);
protected:
  CJavascriptException(v8::TryCatch& try_catch)
    : std::runtime_error(Extract(try_catch)), m_type(NULL),
      m_exc(v8::Persistent<v8::Value>::New(try_catch.Exception())),
      m_msg(v8::Persistent<v8::Message>::New(try_catch.Message())),
      m_stack(v8::Persistent<v8::Value>::New(try_catch.StackTrace()))
  {
    
  }
public:
  CJavascriptException(const std::string& msg, PyObject *type = NULL)
    : std::runtime_error(msg), m_type(type)
  {
  }

  ~CJavascriptException() throw()
  {
    if (!m_exc.IsEmpty()) m_exc.Dispose();
    if (!m_msg.IsEmpty()) m_msg.Dispose();
  }

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

  static void ThrowIf(v8::TryCatch& try_catch)
  {
    if (try_catch.HasCaught()) throw CJavascriptException(try_catch);    
  }

  static void Expose(void);
};