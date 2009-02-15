#include "Wrapper.h"

v8::Handle<v8::Value> CPythonWrapper::Getter(
  v8::Local<v8::String> prop, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Handle<v8::Context> context(pThis->m_context);

  v8::Context::Scope context_scope(context);

  py::object obj = pThis->Unwrap(info.Holder());

  v8::String::AsciiValue name(prop);

  return handle_scope.Close(pThis->Wrap(obj.attr(*name)));
}

v8::Handle<v8::Value> CPythonWrapper::Setter(
  v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
  v8::HandleScope handle_scope;

  CPythonWrapper *pThis = static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(info.Data())->Value());

  v8::Handle<v8::Context> context(pThis->m_context);

  v8::Context::Scope context_scope(context);

  py::object obj = pThis->Unwrap(info.Holder());

  v8::String::AsciiValue name(prop), data(value);

  obj.attr(*name) = *data;

  return handle_scope.Close(pThis->Wrap(obj.attr(*name)));
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
  clazz->SetNamedPropertyHandler(Getter, Setter, 0, 0, 0, cookie);
  clazz->SetCallAsFunctionHandler(Caller, cookie);

  return v8::Persistent<v8::ObjectTemplate>::New(clazz);
}

#define TRY_CONVERT(type, cls) { py::extract<type> value(obj); \
  if (value.check()) return handle_scope.Close(cls::New(value())); }

v8::Handle<v8::Value> CPythonWrapper::Wrap(py::object obj)
{
  v8::HandleScope handle_scope;

  if (obj.ptr() == Py_None) return v8::Null();
  if (obj.ptr() == Py_True) return v8::True();
  if (obj.ptr() == Py_False) return v8::False();

  TRY_CONVERT(int, v8::Int32);
  TRY_CONVERT(const char *, v8::String);
  TRY_CONVERT(bool, v8::Boolean);
  TRY_CONVERT(double, v8::Number);

  v8::Handle<v8::Object> instance = m_template->NewInstance();
  v8::Handle<v8::External> payload = v8::External::New(new py::object(obj));

  instance->SetInternalField(0, payload);

  return handle_scope.Close(instance);
}
py::object CPythonWrapper::Unwrap(v8::Handle<v8::Value> obj)
{
  v8::HandleScope handle_scope;
  
  if (obj->IsNull()) return py::object();
  if (obj->IsTrue()) return py::object(py::handle<>(Py_True));
  if (obj->IsFalse()) return py::object(py::handle<>(Py_False));

  if (obj->IsInt32()) return py::object(obj->Int32Value());  
  if (obj->IsString()) return py::str(*obj->ToString());
  if (obj->IsBoolean()) return py::object(py::handle<>(obj->BooleanValue() ? Py_True : Py_False));
  if (obj->IsNumber()) return py::object(py::handle<>(::PyFloat_FromDouble(obj->NumberValue())));

  if (obj->IsObject())
  {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(obj->ToObject()->GetInternalField(0));

    return *static_cast<py::object *>(field->Value());
  }
  
  throw CWrapperException("unknown object type");
}
