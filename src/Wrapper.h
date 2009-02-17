#pragma once

#include <sstream>

#include <boost/shared_ptr.hpp>

#include "Exception.h"

class CJavascriptObject;

typedef boost::shared_ptr<CJavascriptObject> CJavascriptObjectPtr;

class CWrapper
{  
protected:
  v8::Persistent<v8::Context> m_context;

  CWrapper(v8::Handle<v8::Context> context)
    : m_context(v8::Persistent<v8::Context>::New(context))
  {
    
  }

  ~CWrapper()
  {
    m_context.Dispose();
  }

  py::object Cast(v8::Handle<v8::Value> obj);
  v8::Handle<v8::Value> Cast(py::object obj);
public:
  static void Expose(void);
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
  CPythonWrapper(v8::Handle<v8::Context> context) 
    : CWrapper(context), m_template(SetupTemplate())
  {

  }

  ~CPythonWrapper(void)
  {
    m_template.Dispose();
  }

  v8::Handle<v8::Value> Wrap(py::object obj);
  py::object Unwrap(v8::Handle<v8::Value> obj);
};

class CJavascriptObject : public CWrapper
{
  v8::Persistent<v8::Object> m_obj;

  void CheckAttr(v8::Handle<v8::String> name) const;
public:
  CJavascriptObject(v8::Handle<v8::Context> context, v8::Handle<v8::Object> obj)
    : CWrapper(context), m_obj(v8::Persistent<v8::Object>::New(obj))
  {
  }

  virtual ~CJavascriptObject()
  {
    m_obj.Dispose();
  }

  CJavascriptObjectPtr GetAttr(const std::string& name);
  void SetAttr(const std::string& name, py::object value);
  void DelAttr(const std::string& name);
  
  operator long() const;
  operator double() const;

  CJavascriptObjectPtr Invoke(py::list args);
  
  void dump(std::ostream& os) const;  
};
