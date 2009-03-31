#include "Exception.h"

#include <sstream>

std::ostream& operator<<(std::ostream& os, const CJavascriptException& ex)
{
  os << "JSError: " << ex.what();

  return os;
}

void CJavascriptException::Expose(void)
{
  py::class_<CJavascriptException>("_JSError", py::no_init)
    .def(str(py::self));

  py::register_exception_translator<CJavascriptException>(CJavascriptException::Translator);
}

void CJavascriptException::Translator(CJavascriptException const& ex) 
{
  if (ex.m_exc)
  {
    ::PyErr_SetString(ex.m_exc, ex.what());
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

const std::string ExceptionExtractor::Extract(v8::TryCatch& try_catch)
{
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::String::AsciiValue msg(try_catch.Exception());

  std::ostringstream oss;

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
