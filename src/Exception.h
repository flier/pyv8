#pragma once

#include <cassert>
#include <stdexcept>

#include <v8.h>

#ifdef _WIN32
# pragma warning( push )
# pragma warning( disable : 4100 ) // 'identifier' : unreferenced formal parameter
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

class CPythonException : public std::runtime_error
{
  PyObject *m_exc;
protected:
  CPythonException(const std::string& msg, PyObject *exc) 
    : std::runtime_error(msg), m_exc(exc)
  {
    assert(exc);
  }
public:
  static void translator(CPythonException const& ex) 
  {
    ::PyErr_SetString(ex.m_exc, ex.what());
  }

  static void Expose(void);
};

class CWrapperException : public CPythonException
{
public:
  CWrapperException(const std::string& msg, PyObject *exc = ::PyExc_UserWarning) 
    : CPythonException(msg, exc)
  {

  }
};

class CEngineException : public CPythonException
{
  const std::string generate(v8::TryCatch& try_catch)
  {
    v8::String::AsciiValue exception(try_catch.Exception());

    return *exception;
  }
public:
  CEngineException(v8::TryCatch& try_catch, PyObject *exc = ::PyExc_UserWarning) 
    : CPythonException(generate(try_catch), exc)
  {
  }

  CEngineException(const std::string& msg, PyObject *exc = ::PyExc_UserWarning)
    : CPythonException(msg, exc)
  {
  }
};
