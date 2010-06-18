#include "Context.h"

#include "Wrapper.h"
#include "Engine.h"

void CContext::Expose(void)
{
  py::class_<CContext, boost::noncopyable>("JSContext", py::no_init)
    .def(py::init<py::object, py::list>((py::arg("global") = py::object(), 
                                         py::arg("extensions") = py::list()), 
                                        "create a new context base on global object"))
                  
    .add_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)

    .add_property("locals", &CContext::GetGlobal, "Local variables within context")
    
    .add_static_property("entered", &CContext::GetEntered, 
                         "the last entered context.")
    .add_static_property("current", &CContext::GetCurrent, 
                         "the context that is on the top of the stack.")
    .add_static_property("calling", &CContext::GetCalling,
                         "the context of the calling JavaScript code.")
    .add_static_property("inContext", &CContext::InContext,
                         "Returns true if V8 has a current context.")

    .def("eval", &CContext::Evaluate, (py::arg("source"), 
                                       py::arg("name") = std::string(),
                                       py::arg("line") = -1,
                                       py::arg("col") = -1,
                                       py::arg("precompiled") = py::object()))

    .def("enter", &CContext::Enter, "Enter this context. "
         "After entering a context, all code compiled and "
         "run is compiled and run in this context.")
    .def("leave", &CContext::Leave, "Exit this context. "
         "Exiting the current context restores the context "
         "that was in place when entering the current context.")

    .def("__nonzero__", &CContext::IsEntered)
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CContext>, 
    py::objects::make_ptr_instance<CContext, 
    py::objects::pointer_holder<boost::shared_ptr<CContext>,CContext> > >();
}

CContext::CContext(v8::Handle<v8::Context> context)
{
  v8::HandleScope handle_scope;

  m_context = v8::Persistent<v8::Context>::New(context);
}

CContext::CContext(py::object global, py::list extensions)
{
  v8::HandleScope handle_scope;  

  std::auto_ptr<v8::ExtensionConfiguration> cfg;
  std::vector<std::string> ext_names;
  std::vector<const char *> ext_ptrs;

  for (Py_ssize_t i=0; i<PyList_Size(extensions.ptr()); i++)
  {
    py::extract<const std::string> extractor(::PyList_GetItem(extensions.ptr(), i));

    if (extractor.check()) 
    {
      ext_names.push_back(extractor());      
    }
  }

  for (size_t i=0; i<ext_names.size(); i++)
  {
    ext_ptrs.push_back(ext_names[i].c_str());
  }

  if (!ext_ptrs.empty()) cfg.reset(new v8::ExtensionConfiguration(ext_ptrs.size(), &ext_ptrs[0]));
  
  m_context = v8::Context::New(cfg.get());

  v8::Context::Scope context_scope(m_context);

  if (global.ptr() != Py_None)
  {    
    m_context->Global()->Set(v8::String::NewSymbol("__proto__"), CPythonObject::Wrap(global));  
  }
}

py::object CContext::GetGlobal(void) 
{ 
  v8::HandleScope handle_scope;

  return CJavascriptObject::Wrap(m_context->Global()); 
}

py::str CContext::GetSecurityToken(void)
{
  v8::HandleScope handle_scope;
 
  v8::Handle<v8::Value> token = m_context->GetSecurityToken();

  if (token.IsEmpty()) return py::str();
  
  v8::String::AsciiValue str(token->ToString());

  return py::str(*str, str.length());    
}

void CContext::SetSecurityToken(py::str token)
{
  v8::HandleScope handle_scope;

  if (token.ptr() == Py_None) 
  {
    m_context->UseDefaultSecurityToken();
  }
  else
  {    
    m_context->SetSecurityToken(v8::String::New(py::extract<const char *>(token)()));  
  }
}

py::object CContext::GetEntered(void) 
{ 
  v8::HandleScope handle_scope;

  return !v8::Context::InContext() ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(
      CContextPtr(new CContext(v8::Context::GetEntered())))));
}
py::object CContext::GetCurrent(void) 
{ 
  v8::HandleScope handle_scope;

  return !v8::Context::InContext() ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(
      CContextPtr(new CContext(v8::Context::GetCurrent()))))); 
}
py::object CContext::GetCalling(void)
{
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> calling = v8::Context::GetCalling();

  return calling.IsEmpty() ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(
      CContextPtr(new CContext(handle_scope.Close(calling))))));
}

py::object CContext::Evaluate(const std::string& src, 
                              const std::string name,
                              int line, int col,
                              py::object precompiled) 
{ 
  CEngine engine;

  CScriptPtr script = engine.Compile(src, name, line, col, precompiled);

  return script->Run(); 
}
