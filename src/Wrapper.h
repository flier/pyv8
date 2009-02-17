#pragma once

#include <cassert>
#include <stdexcept>
#include <sstream>

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
  PyObject *m_exc;
protected:
  CPythonException(const std::string& msg, PyObject *exc = ::PyExc_UserWarning) 
    : std::runtime_error(msg), m_exc(exc)
  {

  }
public:
  static void translator(CPythonException const& ex) 
  {
    ::PyErr_SetString(ex.m_exc, ex.what());
  }
};

class CWrapperException : public CPythonException
{
public:
  CWrapperException(const std::string& msg, PyObject *exc = ::PyExc_UserWarning) 
    : CPythonException(msg, exc)
  {

  }
};

class CWrapper
{
protected:
  v8::Persistent<v8::Context> m_context;  

  CWrapper(v8::Handle<v8::Context> context = v8::Handle<v8::Context>()) : m_context(context)
  {

  }

  py::object Cast(v8::Handle<v8::Value> obj);
  v8::Handle<v8::Value> Cast(py::object obj);
public:
  void Attach(v8::Handle<v8::Context> context)
  {
    assert(!context.IsEmpty());
    assert(m_context.IsEmpty());

    v8::HandleScope handle_scope;

    m_context = v8::Persistent<v8::Context>::New(context);
  }
  void Detach()
  {
    assert(!m_context.IsEmpty());

    m_context.Dispose();
  }
};

class CPythonWrapper : public CWrapper
{
  static v8::Handle<v8::Value> Getter(v8::Local<v8::String> prop, 
                                      const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> Setter(v8::Local<v8::String> prop, 
                                      v8::Local<v8::Value> value, 
                                      const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> Caller(const v8::Arguments& args);
protected:
  v8::Persistent<v8::ObjectTemplate> m_template;

  v8::Persistent<v8::ObjectTemplate> SetupTemplate(void);
public:
  CPythonWrapper(void) 
  {    
    m_template = SetupTemplate();
  }
  ~CPythonWrapper(void)
  {
    m_template.Dispose();
  }

  v8::Handle<v8::ObjectTemplate> AsTemplate(void) { return m_template; }

  v8::Handle<v8::Value> Wrap(py::object obj);
  py::object Unwrap(v8::Handle<v8::Value> obj);
};

class CJavascriptObject : public CWrapper
{
  v8::Persistent<v8::Object> m_obj;

  void CheckAttr(v8::Handle<v8::String> name) const;
public:
  CJavascriptObject(v8::Handle<v8::Context> context, v8::Handle<v8::Object> obj)
    : CWrapper(context)
  {
    v8::HandleScope handle_scope;

    m_obj = v8::Persistent<v8::Object>::New(obj);
  }

  virtual ~CJavascriptObject()
  {
    //m_obj.Dispose();
  }

  CJavascriptObject GetAttr(const std::string& name);
  void SetAttr(const std::string& name, py::object value);
  void DelAttr(const std::string& name);

  operator long() const;
  operator double() const;

  CJavascriptObject Invoke(py::list args);
  
  void dump(std::ostream& os) const;

  static void Expose(void);
};