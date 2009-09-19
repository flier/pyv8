#include "Debug.h"

#include <sstream>
#include <string>

py::object CDebug::s_onDebugMessage;

void CDebug::Init(void)
{
  v8::HandleScope scope;

  v8::Handle<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New();

  m_global_context = v8::Context::New(NULL, global_template);
  m_global_context->SetSecurityToken(v8::Undefined());
}

void CDebug::SetEnable(bool enable)
{
  if (m_enabled == enable) return;

  m_enabled = enable;

  v8::HandleScope scope;

  if (enable)
  {
    v8::Handle<v8::External> data = v8::External::New(this);

    v8::Debug::SetDebugEventListener(OnDebugEvent, data);
    v8::Debug::SetMessageHandler(OnDebugMessage);
  }
}

void CDebug::OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
  v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data)
{
  v8::HandleScope scope;  
  
  CDebug *pThis = static_cast<CDebug *>(v8::Handle<v8::External>::Cast(data)->Value());

  if (!pThis->m_enabled) return;

  if (pThis->m_onDebugEvent.ptr() == Py_None) return;

  v8::Context::Scope context_scope(pThis->m_global_context);  

  CJavascriptObjectPtr event_obj(new CJavascriptObject(event_data));

  CPythonGIL python_gil;

  py::call<void>(pThis->m_onDebugEvent.ptr(), event, event_obj);
}

void CDebug::OnDebugMessage(const uint16_t* message, int length, v8::Debug::ClientData* client_data)
{
  if (s_onDebugMessage.ptr() == Py_None) return;

  std::wstring msg(reinterpret_cast<std::wstring::const_pointer>(message), length);

  CPythonGIL python_gil;

  py::call<void>(s_onDebugMessage.ptr(), msg);
}

void CDebug::Expose(void)
{
  py::class_<CDebug, boost::noncopyable>("JSDebug", py::no_init)
    .add_property("enabled", &CDebug::IsEnabled, &CDebug::SetEnable)

    .def_readwrite("onDebugEvent", &CDebug::m_onDebugEvent)
    .attr("onDebugMessage") = CDebug::s_onDebugMessage;

  py::enum_<v8::DebugEvent>("JSDebugEvent")
    .value("Break", v8::Break)
    .value("Exception", v8::Exception)
    .value("NewFunction", v8::NewFunction)
    .value("BeforeCompile", v8::BeforeCompile)
    .value("AfterCompile", v8::AfterCompile)
    ;

  def("debug", &CDebug::GetInstance, 
    py::return_value_policy<py::reference_existing_object>());
}
