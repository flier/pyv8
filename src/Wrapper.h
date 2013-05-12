#pragma once

#include <map>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "Exception.h"

class CJavascriptObject;
class CJavascriptFunction;

typedef boost::shared_ptr<CJavascriptObject> CJavascriptObjectPtr;
typedef boost::shared_ptr<CJavascriptFunction> CJavascriptFunctionPtr;

struct CWrapper
{  
  static void Expose(void);
};

class CPythonObject : public CWrapper
{
  static v8::Handle<v8::Value> NamedGetter(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> NamedSetter(
    v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
  static v8::Handle<v8::Integer> NamedQuery(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> NamedDeleter(
    v8::Local<v8::String> prop, const v8::AccessorInfo& info);
  static v8::Handle<v8::Array> NamedEnumerator(const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> IndexedGetter(
    uint32_t index, const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> IndexedSetter(
    uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
  static v8::Handle<v8::Integer> IndexedQuery(
    uint32_t index, const v8::AccessorInfo& info);
  static v8::Handle<v8::Boolean> IndexedDeleter(
    uint32_t index, const v8::AccessorInfo& info);
  static v8::Handle<v8::Array> IndexedEnumerator(const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> Caller(const v8::Arguments& args);

#ifdef SUPPORT_TRACE_LIFECYCLE
  static void DisposeCallback(v8::Persistent<v8::Value> object, void* parameter);
#endif
protected:
  static void SetupObjectTemplate(v8::Handle<v8::ObjectTemplate> clazz);
  static v8::Persistent<v8::ObjectTemplate> CreateObjectTemplate(void);

  static v8::Handle<v8::Value> WrapInternal(py::object obj);
public:
  static bool IsWrapped(v8::Handle<v8::Object> obj);
  static v8::Handle<v8::Value> Wrap(py::object obj);
  static py::object Unwrap(v8::Handle<v8::Object> obj);
  static void Dispose(v8::Handle<v8::Value> value);

  static void ThrowIf(void);
};

struct ILazyObject
{
  virtual void LazyConstructor(void) = 0;
};

class CJavascriptObject : public CWrapper
{
protected:
  v8::Persistent<v8::Object> m_obj;

  void CheckAttr(v8::Handle<v8::String> name) const;

  CJavascriptObject()
  {

  }
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

  int GetIdentityHash(void);
  CJavascriptObjectPtr Clone(void);

  bool Contains(const std::string& name);
  
  operator long() const;
  operator double() const;
  operator bool() const;  
  
  bool Equals(CJavascriptObjectPtr other) const;
  bool Unequals(CJavascriptObjectPtr other) const { return !Equals(other); }
  
  void Dump(std::ostream& os) const;  

  static py::object Wrap(CJavascriptObject *obj);
  static py::object Wrap(v8::Handle<v8::Value> value,
    v8::Handle<v8::Object> self = v8::Handle<v8::Object>());
  static py::object Wrap(v8::Handle<v8::Object> obj, 
    v8::Handle<v8::Object> self = v8::Handle<v8::Object>());
};

class CJavascriptArray : public CJavascriptObject, public ILazyObject
{
  py::object m_items;
  size_t m_size;
public:
  class ArrayIterator 
    : public boost::iterator_facade<ArrayIterator, py::object const, boost::forward_traversal_tag, py::object>
  {
    CJavascriptArray *m_array;
    size_t m_idx;
  public:
    ArrayIterator(CJavascriptArray *array, size_t idx)
      : m_array(array), m_idx(idx)
    {
    }

    void increment() { m_idx++; }

    bool equal(ArrayIterator const& other) const { return m_array == other.m_array && m_idx == other.m_idx; }

    reference dereference() const { return m_array->GetItem(py::long_(m_idx)); }
  };

  CJavascriptArray(v8::Handle<v8::Array> array)
    : CJavascriptObject(array), m_size(array->Length())
  {

  }

  CJavascriptArray(py::object items) 
    : m_items(items), m_size(0)
  {
  }

  size_t Length(void);

  py::object GetItem(py::object key);
  py::object SetItem(py::object key, py::object value);
  py::object DelItem(py::object key);
  bool Contains(py::object item);

  ArrayIterator begin(void) { return ArrayIterator(this, 0);}
  ArrayIterator end(void) { return ArrayIterator(this, Length());}

  // ILazyObject
  virtual void LazyConstructor(void);
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

  static py::object CallWithArgs(py::tuple args, py::dict kwds);
  static py::object CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds);

  py::object Apply(CJavascriptObjectPtr self, py::list args, py::dict kwds);
  py::object Invoke(py::list args, py::dict kwds);

  const std::string GetName(void) const;
  void SetName(const std::string& name);

  int GetLineNumber(void) const;
  int GetColumnNumber(void) const;
  const std::string GetResourceName(void) const;
  const std::string GetInferredName(void) const;
  int GetLineOffset(void) const;
  int GetColumnOffset(void) const;

  py::object GetOwner(void) const;
};

#ifdef SUPPORT_TRACE_LIFECYCLE

class ObjectTracer;

typedef std::map<PyObject *, ObjectTracer *> LivingMap;

class ObjectTracer
{
  v8::Persistent<v8::Value> m_handle;
  std::auto_ptr<py::object> m_object;

  LivingMap *m_living;
  
  void Trace(void);
  
  static void WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Value> value, void* parameter);

  static LivingMap *GetLivingMapping(void);
public:
  ObjectTracer(v8::Handle<v8::Value> handle, py::object *object);
  ~ObjectTracer(void);

  v8::Persistent<v8::Value> Handle(void) const { return m_handle; }
  py::object *Object(void) const { return m_object.get(); }
  
  void Dispose(void);

  static ObjectTracer& Trace(v8::Handle<v8::Value> handle, py::object *object);

  static v8::Persistent<v8::Value> FindCache(py::object obj);
};

class ContextTracer
{
  v8::Persistent<v8::Context> m_ctxt;
  std::auto_ptr<LivingMap> m_living;
  
  void Trace(void);
  
  static void WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Value> value, void* parameter);
public:
  ContextTracer(v8::Handle<v8::Context> ctxt, LivingMap *living);
  ~ContextTracer(void);
  
  static void Trace(v8::Handle<v8::Context> ctxt, LivingMap *living);
};

#endif
