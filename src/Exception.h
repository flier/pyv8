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

struct ExceptionExtractor
{
  static const std::string Extract(v8::TryCatch& try_catch);
};

template <typename E, typename T = ExceptionExtractor>
struct ExceptionChecker
{
  static void ThrowIf(v8::TryCatch& try_catch)
  {
    if (try_catch.HasCaught())   
      throw E(T::Extract(try_catch));    
  }
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
  PyObject *m_exc;

  friend struct ExceptionTranslator;
public:
  CJavascriptException(const std::string& msg, PyObject *exc = NULL)
    : std::runtime_error(msg), m_exc(exc)
  {
  }

  static void Expose(void);
};

class CEngineException : public CJavascriptException
{
public:
  CEngineException(const std::string& msg) 
    : CJavascriptException(msg)
  {

  }
};
