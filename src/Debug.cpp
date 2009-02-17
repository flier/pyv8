#include "Debug.h"

#include <sstream>
#include <string>

void CDebug::Init(void)
{
  v8::HandleScope scope;

  v8::Handle<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New();
  m_global_context = v8::Context::New(NULL, global_template);
  m_global_context->SetSecurityToken(v8::Undefined());

  m_evaluation_context = v8::Context::New(NULL, global_template);
  m_evaluation_context->SetSecurityToken(v8::Undefined());
}

void CDebug::SetEnable(bool enable)
{
  if (m_enabled == enable) return;

  v8::HandleScope scope;
  v8::Context::Scope context_scope(m_global_context);

  if (enable)
  {
    v8::Handle<v8::External> data = v8::External::New(this);

    v8::Debug::SetDebugEventListener(OnDebugEvent, data);
    v8::Debug::SetMessageHandler(OnDebugMessage, this);
  }
  else
  {
    v8::Debug::SetDebugEventListener(NULL);
    v8::Debug::SetMessageHandler(NULL);
  }

  m_enabled = enable;
}

const CDebugEvent& CDebug::GetDebugEvent(v8::DebugEvent event)
{
  switch (event)
  {
  case v8::Break: return m_onBreak;
  case v8::Exception: return m_onException;
  case v8::NewFunction: return m_onNewFunction;
  case v8::BeforeCompile: return m_onBeforeCompile;
  case v8::AfterCompile: return m_onAfterCompile;
  default: throw CWrapperException("unknown debug event");
  }
}

void CDebug::OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
  v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data)
{
  v8::HandleScope scope;
  
  CDebug *pThis = static_cast<CDebug *>(v8::Handle<v8::External>::Cast(data)->Value());

  v8::Context::Scope context_scope(pThis->m_global_context);

  v8::TryCatch try_catch;

  const CDebugEvent& events = pThis->GetDebugEvent(event);

  CJavascriptObject event_obj(pThis->m_global_context, v8::Persistent<v8::Object>::New(event_data));

  for (size_t i=0; i<events.m_callbacks.size(); i++)
  {    
    py::object callback = events.m_callbacks[i];

    py::call<void>(callback.ptr(), event_obj);
  }
}

void CDebug::OnDebugMessage(const uint16_t* message, int length, void* data)
{
  v8::HandleScope scope;

  CDebug *pThis = static_cast<CDebug *>(data);
  
  v8::Context::Scope context_scope(pThis->m_global_context);

  v8::TryCatch try_catch;

  for (size_t i=0; i<pThis->m_onMessage.m_callbacks.size(); i++)
  {
    py::object callback = pThis->m_onMessage.m_callbacks[i];

    std::wstring msg(reinterpret_cast<std::wstring::const_pointer>(message), length);

    py::call<void>(callback.ptr(), msg);
  }
}

void CDebug::Expose(void)
{
  py::class_<CDebugEvent, boost::noncopyable>("DebugEvent", py::no_init)
    .def(py::self += py::object())
    .def(py::self -= py::other<py::object>())
    ;

  py::class_<CDebug, boost::noncopyable>("Debug", py::no_init)
    .add_property("enabled", &CDebug::IsEnabled, &CDebug::SetEnable)

    .def_readonly("onBreak", &CDebug::m_onBreak)
    .def_readonly("onException", &CDebug::m_onException)
    .def_readonly("onNewFunction", &CDebug::m_onNewFunction)
    .def_readonly("onBeforeCompile", &CDebug::m_onBeforeCompile)
    .def_readonly("onAfterCompile", &CDebug::m_onAfterCompile)
    ;

  def("debug", &CDebug::GetInstance, 
    py::return_value_policy<py::reference_existing_object>());
}
