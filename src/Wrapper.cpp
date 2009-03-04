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
         (py::arg("args") = py::list(), 
          py::arg("kwds") = py::dict()))
    .def("apply", &CJavascriptFunction::Apply, 
         (py::arg("self"), 
          py::arg("args") = py::list(), 
          py::arg("kwds") = py::dict()))
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

v8::Handle<v8::Value> CPythonObject::NamedGetter(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  if (!::PyObject_HasAttr(obj.ptr(), attr_name.ptr()))
    return v8::Local<v8::Value>();

  return handle_scope.Close(Wrap(obj.attr(*name)));
}

v8::Handle<v8::Value> CPythonObject::NamedSetter(
  v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());

  v8::String::AsciiValue name(prop), data(value);

  obj.attr(*name) = *data;

  return value;
}

v8::Handle<v8::Boolean> CPythonObject::NamedQuery(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  return v8::Boolean::New(::PyObject_HasAttr(obj.ptr(), attr_name.ptr()));
}

v8::Handle<v8::Boolean> CPythonObject::NamedDeleter(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  v8::String::AsciiValue name(prop);

  py::str attr_name(*name, name.length());

  return v8::Boolean::New(::PyObject_DelAttr(obj.ptr(), attr_name.ptr()));
}

v8::Handle<v8::Value> CPythonObject::IndexedGetter(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  py::object ret(py::handle<>(::PySequence_GetItem(obj.ptr(), index)));

  return handle_scope.Close(Wrap(ret));  
}
v8::Handle<v8::Value> CPythonObject::IndexedSetter(
  uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  int ret = ::PySequence_SetItem(obj.ptr(), index, Unwrap(value).ptr());

  if (ret < 0) throw CWrapperException("fail to set indexed value");

  return value;
}
v8::Handle<v8::Boolean> CPythonObject::IndexedQuery(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  return v8::Boolean::New(index < ::PySequence_Size(obj.ptr()));
}
v8::Handle<v8::Boolean> CPythonObject::IndexedDeleter(
  uint32_t index, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  py::object obj = Unwrap(info.Holder());  

  v8::Handle<v8::Value> value = IndexedGetter(index, info);

  return v8::Boolean::New(0 <= ::PySequence_DelItem(obj.ptr(), index));
}

v8::Handle<v8::Value> CPythonObject::Caller(const v8::Arguments& args)
{
  v8::HandleScope handle_scope;

  py::object self;
  
  if (args.Data().IsEmpty())
  {
    self = Unwrap(args.This());
  }
  else
  {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(args.Data());

    self = *static_cast<py::object *>(field->Value());
  }

  py::object result;

  switch (args.Length())
  {
  case 0: result = self(); break;
  case 1: result = self(Unwrap(args[0])); break;
  case 2: result = self(Unwrap(args[0]), Unwrap(args[1])); break;
  case 3: result = self(Unwrap(args[0]), Unwrap(args[1]), 
                        Unwrap(args[2])); break;
  case 4: result = self(Unwrap(args[0]), Unwrap(args[1]), 
                        Unwrap(args[2]), Unwrap(args[3])); break;
  case 5: result = self(Unwrap(args[0]), Unwrap(args[1]), 
                        Unwrap(args[2]), Unwrap(args[3]),
                        Unwrap(args[4])); break;
  case 6: result = self(Unwrap(args[0]), Unwrap(args[1]), 
                        Unwrap(args[2]), Unwrap(args[3]),
                        Unwrap(args[4]), Unwrap(args[5])); break;
  default:
    throw CWrapperException("too many arguments");
  }

  return handle_scope.Close(Wrap(result));
}

void CPythonObject::SetupObjectTemplate(v8::Handle<v8::ObjectTemplate> clazz)
{
  clazz->SetInternalFieldCount(1);
  clazz->SetNamedPropertyHandler(NamedGetter, NamedSetter, NamedQuery, NamedDeleter);
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter);
  clazz->SetCallAsFunctionHandler(Caller);
}

v8::Persistent<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(void)
{
  v8::HandleScope handle_scope;

  v8::Handle<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New();

  SetupObjectTemplate(clazz);

  return v8::Persistent<v8::ObjectTemplate>::New(clazz);
}

v8::Handle<v8::Value> CPythonObject::Wrap(py::object obj)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::Handle<v8::Value> result = Cast(obj);

  if (!result->IsUndefined()) return handle_scope.Close(result);

  v8::Handle<v8::Object> instance;

  if (PyFunction_Check(obj.ptr()) || PyMethod_Check(obj.ptr()))
  {
    v8::Handle<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New();    

    func_tmpl->SetCallHandler(Caller, v8::External::New(new py::object(obj)));
    
    instance = func_tmpl->GetFunction();
  }
  else
  {
    static v8::Persistent<v8::ObjectTemplate> s_template = CreateObjectTemplate();

    instance = s_template->NewInstance();

    v8::Handle<v8::External> payload = v8::External::New(new py::object(obj));

    instance->SetInternalField(0, payload);
  }

  return handle_scope.Close(instance);
}
py::object CPythonObject::Unwrap(v8::Handle<v8::Value> obj)
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
  else if (obj->IsFunction())
  {
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(obj);
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(func->GetInternalField(0));

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

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());

  CheckAttr(attr_name);

  v8::Handle<v8::Value> attr_obj = m_obj->Get(attr_name);

  if (attr_obj.IsEmpty()) CWrapperException::Throw(try_catch);

  return CJavascriptObject::Wrap(attr_obj->ToObject(), m_obj);
}

void CJavascriptObject::SetAttr(const std::string& name, py::object value)
{
  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());
  v8::Handle<v8::Value> attr_obj = Cast(value);

  if (attr_obj->IsUndefined())
  {
    py::extract<CJavascriptObject&> extractor(value);

    if (extractor.check())
    {
      attr_obj = extractor().m_obj;
    }
  }

  if (!m_obj->Set(attr_name, attr_obj)) CWrapperException::Throw(try_catch);
}
void CJavascriptObject::DelAttr(const std::string& name)
{
  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::String> attr_name = v8::String::New(name.c_str());

  CheckAttr(attr_name);
  
  if (!m_obj->Delete(attr_name)) CWrapperException::Throw(try_catch);
}

bool CJavascriptObject::Equals(CJavascriptObjectPtr other) const
{
  v8::HandleScope handle_scope;

  return m_obj->Equals(other->m_obj);
}

void CJavascriptObject::Dump(std::ostream& os) const
{
  v8::HandleScope handle_scope;

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

  if (m_obj.IsEmpty())
    throw CWrapperException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return m_obj->Int32Value(); 
}
CJavascriptObject::operator double() const 
{ 
  v8::HandleScope handle_scope;

  if (m_obj.IsEmpty())
    throw CWrapperException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return m_obj->NumberValue(); 
}

CJavascriptObject::operator bool() const
{
  v8::HandleScope handle_scope;

  if (m_obj.IsEmpty()) return false;

  return m_obj->BooleanValue();
}

CJavascriptObjectPtr CJavascriptObject::Wrap(v8::Handle<v8::Object> obj, v8::Handle<v8::Object> self) 
{
  v8::HandleScope handle_scope;

  if (obj.IsEmpty()) 
  {
    obj = v8::Null()->ToObject();
  }
  else if (obj->IsFunction())
  {
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(obj);

    return CJavascriptObjectPtr(new CJavascriptFunction(self, func));
  }  

  return CJavascriptObjectPtr(new CJavascriptObject(obj));
}

CJavascriptObjectPtr CJavascriptFunction::Call(v8::Handle<v8::Object> self, py::list args, py::dict kwds)
{
  v8::HandleScope handle_scope;

  v8::TryCatch try_catch;

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(m_obj);

  std::vector< v8::Handle<v8::Value> > params(::PyList_Size(args.ptr()));

  for (size_t i=0; i<params.size(); i++)
  {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  v8::Handle<v8::Value> result = func->Call(self,
    params.size(), params.empty() ? NULL : &params[0]);

  if (result.IsEmpty()) CWrapperException::Throw(try_catch);

  return CJavascriptObject::Wrap(result->ToObject());
}
CJavascriptObjectPtr CJavascriptFunction::Apply(CJavascriptObjectPtr self, py::list args, py::dict kwds)
{
  v8::HandleScope handle_scope;

  return Call(self.get() ? self->Object() : v8::Context::GetCurrent()->Global(), args, kwds);
}

CJavascriptObjectPtr CJavascriptFunction::Invoke(py::list args, py::dict kwds) 
{ 
  v8::HandleScope handle_scope;

  return Call(m_self, args, kwds); 
}

const std::string CJavascriptFunction::GetName(void) const
{
  v8::HandleScope handle_scope;

  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(m_obj);  

  v8::String::AsciiValue name(v8::Handle<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}
