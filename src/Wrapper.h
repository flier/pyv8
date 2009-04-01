#pragma once

#include <sstream>

#include <boost/shared_ptr.hpp>

#include "Exception.h"

class CJavascriptObject;

typedef boost::shared_ptr<CJavascriptObject> CJavascriptObjectPtr;

struct CWrapper
{  
  static void Expose(void);
};

class CPythonObject : public CWrapper
{
  static void ThrowIf(void);

  static v8::Handle<v8::Value> NamedGetter(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> NamedSetter(
    v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> NamedQuery(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> NamedDeleter(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> IndexedGetter(
    uint32_t index, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> IndexedSetter(
    uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> IndexedQuery(
    uint32_t index, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> IndexedDeleter(
    uint32_t index, const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> Caller(const v8::Arguments& args);
protected:
  static void SetupObjectTemplate(v8::Handle<v8::ObjectTemplate> clazz);
  static v8::Persistent<v8::ObjectTemplate> CreateObjectTemplate(void);
public:
  static v8::Handle<v8::Value> Wrap(py::object obj);
};

class CJavascriptObject : public CWrapper
{
protected:
  v8::Persistent<v8::Object> m_obj;

  void CheckAttr(v8::Handle<v8::String> name) const;

  static py::object Wrap(CJavascriptObject *obj);
public:
  CJavascriptObject(v8::Handle<v8::Object> obj)
    : m_obj(v8::Persistent<v8::Object>::New(obj))
  {
  }

  virtual ~CJavascriptObject()
  {
    m_obj.Dispose();
  }

  v8::Handle<v8::Object> Object(void) { return m_obj; }
  long Native(void) { return reinterpret_cast<long>(*m_obj); }

  py::object GetAttr(const std::string& name);
  void SetAttr(const std::string& name, py::object value);
  void DelAttr(const std::string& name);

  py::list GetAttrList(void);
  
  operator long() const;
  operator double() const;
  operator bool() const;  
  
  bool Equals(CJavascriptObjectPtr other) const;
  bool Unequals(CJavascriptObjectPtr other) const { return !Equals(other); }
  
  void Dump(std::ostream& os) const;  

  static py::object Wrap(v8::Handle<v8::Value> value,
    v8::Handle<v8::Object> self = v8::Handle<v8::Object>());
  static py::object Wrap(v8::Handle<v8::Object> obj, 
    v8::Handle<v8::Object> self = v8::Handle<v8::Object>());
};

class CJavascriptArray : public CJavascriptObject
{
public:
  CJavascriptArray(v8::Handle<v8::Array> array)
    : CJavascriptObject(array)
  {

  }

  size_t Length(void) const;

  py::object GetItem(size_t idx);
  py::object SetItem(size_t idx, py::object value);
  py::object DelItem(size_t idx);
};

class CJavascriptFunction : public CJavascriptObject
{
  v8::Persistent<v8::Object> m_self;

  py::object Call(v8::Handle<v8::Object> self, py::list args, py::dict kwds);
public:
  CJavascriptFunction(v8::Handle<v8::Object> self, v8::Handle<v8::Function> func)
    : CJavascriptObject(func), m_self(v8::Persistent<v8::Object>::New(self))
  {
  }

  ~CJavascriptFunction()
  {
    m_self.Dispose();
  }
  
  py::object Apply(CJavascriptObjectPtr self, py::list args, py::dict kwds);
  py::object Invoke(py::list args, py::dict kwds);

  const std::string GetName(void) const;
  py::object GetOwner(void) const { return CJavascriptObject::Wrap(m_self); }
};
