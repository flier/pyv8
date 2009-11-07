#include "Exception.h"

#include <sstream>

#undef COMPILER


#include "src/v8.h"

#include "src/bootstrapper.h"
#include "src/natives.h"

#include "Locker.h"

CPythonGIL::CPythonGIL() 
{
  while (CLocker::IsPreemption() && _PyThreadState_Current && ::PyGILState_GetThisThreadState() != ::_PyThreadState_Current)
  {
    v8::internal::Thread::YieldCPU();
  }

  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL()
{
  ::PyGILState_Release(m_state);
}  

std::ostream& operator<<(std::ostream& os, const CJavascriptException& ex)
{
  os << "JSError: " << ex.what();

  return os;
}

void CJavascriptException::Expose(void)
{
  py::class_<CJavascriptException>("_JSError", py::no_init)
    .def(str(py::self))

    .def_readonly("name", &CJavascriptException::GetName)
    .def_readonly("message", &CJavascriptException::GetMessage)
    .def_readonly("scriptName", &CJavascriptException::GetScriptName)
    .def_readonly("lineNum", &CJavascriptException::GetLineNumber)
    .def_readonly("startPos", &CJavascriptException::GetStartPosition)
    .def_readonly("endPos", &CJavascriptException::GetEndPosition)
    .def_readonly("startCol", &CJavascriptException::GetStartColumn)
    .def_readonly("endCol", &CJavascriptException::GetEndColumn)
    .def_readonly("sourceLine", &CJavascriptException::GetSourceLine)
    .def_readonly("stackTrace", &CJavascriptException::GetStackTrace)
    .def("print_tb", &CJavascriptException::PrintCallStack);

  py::register_exception_translator<CJavascriptException>(ExceptionTranslator::Translate);

  py::converter::registry::push_back(ExceptionTranslator::Convertible,
    ExceptionTranslator::Construct, py::type_id<CJavascriptException>());
}
const std::string CJavascriptException::GetName(void) 
{
  if (m_exc.IsEmpty()) return std::string();

  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::String::AsciiValue msg(v8::Handle<v8::String>::Cast(m_exc->ToObject()->Get(v8::String::New("name"))));

  return std::string(*msg, msg.length());
}
const std::string CJavascriptException::GetMessage(void) 
{
  if (m_exc.IsEmpty()) return std::string();

  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::String::AsciiValue msg(v8::Handle<v8::String>::Cast(m_exc->ToObject()->Get(v8::String::New("message"))));

  return std::string(*msg, msg.length());
}
const std::string CJavascriptException::GetScriptName(void) 
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  if (!m_msg.IsEmpty() && !m_msg->GetScriptResourceName().IsEmpty() &&
      !m_msg->GetScriptResourceName()->IsUndefined())
  {
    v8::String::AsciiValue name(m_msg->GetScriptResourceName());

    return std::string(*name, name.length());
  }

  return std::string();
}
int CJavascriptException::GetLineNumber(void) 
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  return m_msg.IsEmpty() ? 1 : m_msg->GetLineNumber();
}
int CJavascriptException::GetStartPosition(void) 
{
  assert(v8::Context::InContext());

  return m_msg.IsEmpty() ? 1 : m_msg->GetStartPosition();
}
int CJavascriptException::GetEndPosition(void)
{
  assert(v8::Context::InContext());

  return m_msg.IsEmpty() ? 1 : m_msg->GetEndPosition();
}
int CJavascriptException::GetStartColumn(void) 
{
  assert(v8::Context::InContext());

  return m_msg.IsEmpty() ? 1 : m_msg->GetStartColumn();
}
int CJavascriptException::GetEndColumn(void) 
{
  assert(v8::Context::InContext());

  return m_msg.IsEmpty() ? 1 : m_msg->GetEndColumn();
}
const std::string CJavascriptException::GetSourceLine(void) 
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  if (!m_msg.IsEmpty() && !m_msg->GetSourceLine().IsEmpty() &&
      !m_msg->GetSourceLine()->IsUndefined())
  {
    v8::String::AsciiValue line(m_msg->GetSourceLine());

    return std::string(*line, line.length());
  }

  return std::string();
}
const std::string CJavascriptException::GetStackTrace(void)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  if (!m_stack.IsEmpty())
  {
    v8::String::AsciiValue stack(v8::Handle<v8::String>::Cast(m_stack));

    return std::string(*stack, stack.length());
  }

  return std::string();
}
const std::string CJavascriptException::Extract(v8::TryCatch& try_catch)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  std::ostringstream oss;

  v8::String::AsciiValue msg(try_catch.Exception());

  if (*msg)
    oss << std::string(*msg, msg.length());

  v8::Handle<v8::Message> message = try_catch.Message();

  if (!message.IsEmpty())
  {
    oss << " ( ";

    if (!message->GetScriptResourceName().IsEmpty() &&
        !message->GetScriptResourceName()->IsUndefined())
    {
      v8::String::AsciiValue name(message->GetScriptResourceName());

      oss << std::string(*name, name.length());
    }

    oss << " @ " << message->GetLineNumber() << " : " << message->GetStartColumn() << " ) ";
    
    if (!message->GetSourceLine().IsEmpty() &&
        !message->GetSourceLine()->IsUndefined())
    {
      v8::String::AsciiValue line(message->GetSourceLine());

      oss << " -> " << std::string(*line, line.length());
    }
  }

  return oss.str();
}

void ExceptionTranslator::Translate(CJavascriptException const& ex) 
{
  CPythonGIL python_gil;

  if (ex.m_type)
  {
    ::PyErr_SetString(ex.m_type, ex.what());
  }
  else
  {
    // Boost::Python doesn't support inherite from Python class,
    // so, just use some workaround to throw our custom exception
    //
    // http://www.language-binding.net/pyplusplus/troubleshooting_guide/exceptions/exceptions.html

    py::object impl(ex);
    py::object clazz = impl.attr("_jsclass");
    py::object err = clazz(impl);

    ::PyErr_SetObject(clazz.ptr(), py::incref(err.ptr()));
  }
}

void *ExceptionTranslator::Convertible(PyObject* obj)
{
  CPythonGIL python_gil;

  if (1 != ::PyObject_IsInstance(obj, ::PyExc_Exception))
    return NULL;

  if (1 != ::PyObject_HasAttrString(obj, "_impl"))
    return NULL;

  py::object err(py::handle<>(py::borrowed(obj)));
  py::object impl = err.attr("_impl");
  py::extract<CJavascriptException> extractor(impl);

  return extractor.check() ? obj : NULL;
}

void ExceptionTranslator::Construct(PyObject* obj, 
  py::converter::rvalue_from_python_stage1_data* data)
{
  CPythonGIL python_gil;

  py::object err(py::handle<>(py::borrowed(obj)));
  py::object impl = err.attr("_impl");

  typedef py::converter::rvalue_from_python_storage<CJavascriptException> storage_t;

  storage_t* the_storage = reinterpret_cast<storage_t*>(data);
  void* memory_chunk = the_storage->storage.bytes;
  CJavascriptException* cpp_err = 
    new (memory_chunk) CJavascriptException(py::extract<CJavascriptException>(impl));

  data->convertible = memory_chunk;
}

void CJavascriptException::PrintCallStack(py::object file)
{  
  CPythonGIL python_gil;

  m_msg->PrintCurrentStackTrace(::PyFile_AsFile(file.ptr()));
}