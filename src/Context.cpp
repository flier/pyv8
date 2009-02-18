#include "Context.h"

#include "Wrapper.h"

void CContext::Expose(void)
{
  py::class_<CContext, py::bases<CJavascriptObject> >("JSContext", py::no_init)
    .def("create", &CContext::Create, (py::arg("global") = py::object()), 
         "create a new context base on global object")
    .staticmethod("create")

    .add_static_property("entered", CContext::GetEntered, 
                         "Returns the last entered context.")
    .add_static_property("current", CContext::GetCurrent, 
                         "Returns the context that is on the top of the stack.")
    .add_static_property("inContext", CContext::InContext,
                         "Returns true if V8 has a current context.")

    .def("enter", &CContext::Enter, "Enter this context. "
         "After entering a context, all code compiled and "
         "run is compiled and run in this context.")
    .def("leave", &CContext::Leave, "Exit this context. "
         "Exiting the current context restores the context "
         "that was in place when entering the current context.")

    .def("__enter__", &CContext::Enter)
    .def("__leave__", &CContext::LeaveWith)
    ;

  py::objects::class_value_wrapper<boost::shared_ptr<CContext>, 
    py::objects::make_ptr_instance<CContext, 
    py::objects::pointer_holder<boost::shared_ptr<CContext>,CContext> > >();
}

CPythonWrapper *CContext::GetWrapper(v8::Handle<v8::Context> context) 
{    
  assert(v8::Context::InContext());

  v8::HandleScope handle_scope;

  v8::Handle<v8::Value> wrapper = context->Global()->Get(v8::String::NewSymbol("__wrapper__"));

  return static_cast<CPythonWrapper *>(v8::Handle<v8::External>::Cast(wrapper)->Value());
}

CContextPtr CContext::Create(py::object global)
{
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = v8::Context::New(NULL, v8::ObjectTemplate::New());

  v8::Context::Scope context_scope(context);

  CContextPtr ctxt(new CContext(context));

  std::auto_ptr<CPythonWrapper> wrapper(new CPythonWrapper(context));

  if (global.ptr() != Py_None)
    context->Global()->Set(v8::String::NewSymbol("__proto__"), wrapper->Wrap(global));  

  context->Global()->Set(v8::String::NewSymbol("__wrapper__"), v8::External::New(wrapper.release()));  

  return ctxt;
}
