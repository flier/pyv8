#include "Wrapper.h"

#include <vector>

#include "Context.h"

std::ostream& operator <<(std::ostream& os, const CJavascriptObject& obj)
{ 
  obj.Dump(os);

  return os;
}

void CWrapper::Expose(void)
{
  py::class_<CJavascriptObject>("JSObject", py::no_init)
    .def_readonly("__js__", &CJavascriptObject::Native)

    .def("__getattr__", &CJavascriptObject::GetAttr)
    .def("__setattr__", &CJavascriptObject::SetAttr)
    .def("__delattr__", &CJavascriptObject::DelAttr)    

    .def(int_(py::self))
    .def(float_(py::self))
    .def(str(py::self))

    .def("__nonzero__", &CJavascriptObject::operator bool)
    .def("__eq__", &CJavascriptObject::Equals)
    .def("__ne__", &CJavascriptObject::Unequals)
    ;

  py::class_<CJavascriptFunction, py::bases<CJavascriptObject> >("JSFunction", py::no_init)
    .def("__call__", &CJavascriptFunction::Invoke, 
         (py::arg("args") = py::list(), py::arg("kwds") = py::dict()))
    .add_property("func_name", &CJavascriptFunction::GetName)
    .add_property("func_owner", &CJavascriptFunction::GetOwner)
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CJavascriptObject>, 
    py::objects::make_ptr_instance<CJavascriptObject, 
    py::objects::pointer_holder<boost::shared_ptr<CJavascriptObject>,CJavascriptObject> > >();
}

py::object CWrapper::Cast(v8::Handle<v8::Value> obj)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  if (obj->IsNull()) return py::object();
  if (obj->IsTrue()) return py::object(py::handle<>(Py_True));
  if (obj->IsFalse()) return py::object(py::handle<>(Py_False));

  if (obj->IsInt32()) return py::object(obj->Int32Value());  
  if (obj->IsString())
  {
    v8::String::AsciiValue str(v8::Handle<v8::String>::Cast(obj));

    return py::str(*str, str.length());
  }
  if (obj->IsBoolean()) return py::object(py::handle<>(obj->BooleanValue() ? Py_True : Py_False));
  if (obj->IsNumber()) return py::object(py::handle<>(::PyFloat_FromDouble(obj->NumberValue())));

  return py::object();
}

#define TRY_CONVERT(type, cls) { py::extract<type> value(obj); \
  if (value.check()) return handle_scope.Close(cls::New(value())); }

v8::Handle<v8::Value> CWrapper::Cast(py::object obj)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  if (obj.ptr() == Py_None) return v8::Null();
  if (obj.ptr() == Py_True) return v8::True();
  if (obj.ptr() == Py_False) return v8::False();

  TRY_CONVERT(int, v8::Int32);
  TRY_CONVERT(const char *, v8::String);
  TRY_CONVERT(bool, v8::Boolean);
  TRY_CONVERT(double, v8::Number);

  return v8::Undefined();
}

v8::Handle<v8::Value> CPythonWrapper::NamedGetter(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  if (!::PyObject_HasAttr(obj.ptr(), attr_name.ptr()))
    return v8::Local<v8::Value>();

  return handle_scope.Close(pThis->Wrap(obj.attr(*name)));
}

v8::Handle<v8::Value> CPythonWrapper::NamedSetter(
  v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());

  v8::String::AsciiValue name(prop), data(value);

  obj.attr(*name) = *data;

  return value;
}

v8::Handle<v8::Boolean> CPythonWrapper::NamedQuery(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  return v8::Boolean::New(::PyObject_HasAttr(obj.ptr(), attr_name.ptr()));
}

v8::Handle<v8::Boolean> CPythonWrapper::NamedDeleter(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  return v8::Boolean::New(::PyObject_DelAttr(obj.ptr(), attr_name.ptr()));
}

v8::Handle<v8::Value> CPythonWrapper::IndexedGetter(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  py::object ret(py::handle<>(::PySequence_GetItem(obj.ptr(), index)));

  return handle_scope.Close(pThis->Wrap(ret));  
}
v8::Handle<v8::Value> CPythonWrapper::IndexedSetter(
  uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  int ret = ::PySequence_SetItem(obj.ptr(), index, pThis->Unwrap(value).ptr());

  if (ret < 0) throw CWrapperException("fail to set indexed value");

  return value;
}
v8::Handle<v8::Boolean> CPythonWrapper::IndexedQuery(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  return v8::Boolean::New(index < ::PySequence_Size(obj.ptr()));
}
v8::Handle<v8::Boolean> CPythonWrapper::IndexedDeleter(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Context::Scope context_scope(pThis->m_context);

  py::object obj = pThis->Unwrap(info.Holder());  

  v8::Handle<v8::Value> value = pThis->IndexedGetter(index, info);

  return v8::Boolean::New(0 <= ::PySequence_DelItem(obj.ptr(), index));
}

v8::Handle<v8::Value> CPythonWrapper::Caller(const v8::Arguments& args)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

  v8::Handle<v8::Context> context(pThis->m_context);

  py::object self = pThis->Unwrap(args.This());

  py::object result;

  switch (args.Length())
  {
  case 0: result = self(); break;
  case 1: result = self(pThis->Unwrap(args[0])); break;
  case 2: result = self(pThis->Unwrap(args[0]), pThis->Unwrap(args[1])); break;
  case 3: result = self(pThis->Unwrap(args[0]), pThis->Unwrap(args[1]), 
                        pThis->Unwrap(args[2])); break;
  case 4: result = self(pThis->Unwrap(args[0]), pThis->Unwrap(args[1]), 
                        pThis->Unwrap(args[2]), pThis->Unwrap(args[3])); break;
  case 5: result = self(pThis->Unwrap(args[0]), pThis->Unwrap(args[1]), 
                        pThis->Unwrap(args[2]), pThis->Unwrap(args[3]),
                        pThis->Unwrap(args[4])); break;
  case 6: result = self(pThis->Unwrap(args[0]), pThis->Unwrap(args[1]), 
                        pThis->Unwrap(args[2]), pThis->Unwrap(args[3]),
                        pThis->Unwrap(args[4]), pThis->Unwrap(args[5])); break;
  default:
    throw CWrapperException("too many arguments");
  }

  return pThis->Wrap(result);
}

v8::Persistent<v8::ObjectTemplate> CPythonWrapper::SetupTemplate(void)
{
  v8::HandleScope handle_scope;

  v8::Handle<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New();
 
  v8::Handle<v8::External> cookie = v8::External::New(this);

  clazz->SetInternalFieldCount(1);
  clazz->SetNamedPropertyHandler(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, 0, cookie);
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, 0, cookie);
  clazz->SetCallAsFunctionHandler(Caller, cookie);

  return v8::Persistent<v8::ObjectTemplate>::New(clazz);
}

v8::Handle<v8::Value> CPythonWrapper::Wrap(py::object obj)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::Handle<v8::Value> result = Cast(obj);

  if (!result->IsUndefined()) return handle_scope.Close(result);

  v8::Handle<v8::Object> instance = m_template->NewInstance();
  v8::Handle<v8::External> payload = v8::External::New(new py::object(obj));

  instance->SetInternalField(0, payload);

  return handle_scope.Close(instance);
}
py::object CPythonWrapper::Unwrap(v8::Handle<v8::Value> obj)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;
  
  py::object result = Cast(obj);

  if (result) return result;

  if (obj->IsObject())
  {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(obj->ToObject()->GetInternalField(0));

    return *static_cast<py::object *>(field->Value());
  }
  
  throw CWrapperException("unknown object type");
}

void CJavascriptObject::CheckAttr(v8::Handle<v8::String> name) const
{
  assert(v8::Context::InContext());

  if (!m_obj->Has(name))
  {
    std::ostringstream msg;
      
    msg << "'" << *v8::String::AsciiValue(m_obj->ObjectProtoToString()) 
        << "' object has no attribute '" << *v8::String::AsciiValue(name) << "'";

    throw CWrapperException(msg.str(), ::PyExc_AttributeError);
  }
}

CJavascriptObjectPtr CJavascriptObject::GetAttr(const std::string& name)
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context); 

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());

  CheckAttr(attr_name);

  v8::Handle<v8::Value> attr_obj = m_obj->Get(attr_name);

  if (attr_obj.IsEmpty()) CWrapperException::Throw(try_catch);

  return CJavascriptObject::Wrap(m_context, attr_obj->ToObject(), m_obj);
}

void CJavascriptObject::SetAttr(const std::string& name, py::object value)
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());

  CheckAttr(attr_name);

  v8::Handle<v8::Value> attr_obj = Cast(value);

  if (!m_obj->Set(attr_name, attr_obj)) CWrapperException::Throw(try_catch);
}
void CJavascriptObject::DelAttr(const std::string& name)
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());

  CheckAttr(attr_name);
  
  if (!m_obj->Delete(attr_name)) CWrapperException::Throw(try_catch);
}

bool CJavascriptObject::Equals(CJavascriptObjectPtr other) const
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  return m_obj->Equals(other->m_obj);
}

void CJavascriptObject::Dump(std::ostream& os) const
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  if (m_obj.IsEmpty())
    os << "None";
  else if (m_obj->IsInt32())
    os << m_obj->Int32Value();
  else if (m_obj->IsNumber())
    os << m_obj->NumberValue();
  else if (m_obj->IsBoolean())
    os << m_obj->BooleanValue();
  else if (m_obj->IsNull())
    os << "None";
  else if (m_obj->IsUndefined())
    os << "N/A";
  else if (m_obj->IsString())  
    os << *v8::String::AsciiValue(v8::Handle<v8::String>::Cast(m_obj));  
  else 
    os << *v8::String::AsciiValue(m_obj->ToString());
}

CJavascriptObject::operator long() const 
{ 
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context); 
  
  return m_obj->Int32Value(); 
}
CJavascriptObject::operator double() const 
{ 
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context); 
  
  return m_obj->NumberValue(); 
}

CJavascriptObject::operator bool() const
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context); 

  return m_obj->BooleanValue();
}

CJavascriptObjectPtr CJavascriptObject::Wrap(v8::Handle<v8::Context> context,
  v8::Handle<v8::Object> obj, v8::Handle<v8::Object> self) 
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(context); 

  if (obj.IsEmpty()) 
  {
    obj = v8::Null()->ToObject();
  }
  else if (obj->IsFunction())
  {
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(obj);

    return CJavascriptObjectPtr(new CJavascriptFunction(context, 
      self.IsEmpty() ? context->Global() : self, func));
  }  

  return CJavascriptObjectPtr(new CJavascriptObject(context, obj));
}

CJavascriptObjectPtr CJavascriptFunction::Invoke(py::list args, py::dict kwds)
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::TryCatch try_catch;

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(m_obj);

  std::vector< v8::Handle<v8::Value> > params(::PyList_Size(args.ptr()));

  for (size_t i=0; i<params.size(); i++)
  {
    params[i] = Cast(args[i]);
  }

  v8::Handle<v8::Value> result = func->Call(m_self,
    params.size(), params.empty() ? NULL : &params[0]);

  if (result.IsEmpty()) CWrapperException::Throw(try_catch);

  return CJavascriptObject::Wrap(m_context, result->ToObject());
}

const std::string CJavascriptFunction::GetName(void) const
{
  v8::HandleScope handle_scope;

  v8::Context::Scope context_scope(m_context);

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(m_obj);  

  v8::String::AsciiValue name(v8::Handle<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}
