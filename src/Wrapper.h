#pragma once

#include <cassert>
#include <stdexcept>

#ifdef _WIN32
# pragma warning( push )
# pragma warning( disable : 4100 ) // 'identifier' : unreferenced formal parameter
# pragma warning( disable : 4244 ) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
# pragma warning( disable : 4512 ) // 'class' : assignment operator could not be generated
#endif

#include <boost/python.hpp>
namespace py = boost::python;

#include <v8.h>

#ifdef _WIN32
# pragma comment( lib, "v8.lib" )
# pragma comment( lib, "v8_base.lib" )
# pragma comment( lib, "v8_snapshot.lib" )

# pragma warning( pop )
#endif

class CPythonException : public std::runtime_error
{
protected:
  CPythonException(const std::string& msg) : std::runtime_error(msg)
  {

  }
public:
  static void translator(CPythonException const& ex) 
  {
    ::PyErr_SetString(PyExc_UserWarning, ex.what());
  }
};

class CWrapperException : public CPythonException
{
public:
  CWrapperException(const char *msg) : CPythonException(msg)
  {

  }
};

class CPythonWrapper
{
  static v8::Handle<v8::Value> Getter(v8::Local<v8::String> prop, 
                                      const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> Setter(v8::Local<v8::String> prop, 
                                      v8::Local<v8::Value> value, 
                                      const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> Caller(const v8::Arguments& args);
protected:
  v8::Persistent<v8::ObjectTemplate> m_template;
  v8::Context *m_context;  

  v8::Persistent<v8::ObjectTemplate> SetupTemplate(void);
public:
  CPythonWrapper(void) : m_context(NULL)
  {    
    m_template = SetupTemplate();
  }
  ~CPythonWrapper(void)
  {
    m_template.Dispose();
  }

  void Attach(v8::Context *context)
  {
    assert(context);
    assert(!m_context);

    m_context = context;
  }
  void Detach()
  {
    assert(m_context);

    m_context = NULL;
  }

  v8::Handle<v8::ObjectTemplate> AsTemplate(void) { return m_template; }

  v8::Handle<v8::Value> Wrap(py::object obj);
  py::object Unwrap(v8::Handle<v8::Value> obj);
};
