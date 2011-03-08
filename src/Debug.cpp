#include "Debug.h"

#include <sstream>
#include <string>

#include "Context.h"

void CDebug::Init(void)
{
  v8::HandleScope scope;

  v8::Handle<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New();

  m_debug_context = v8::Context::New(NULL, global_template);
  m_debug_context->SetSecurityToken(v8::Undefined());
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
    v8::Debug::SetDebugMessageDispatchHandler(OnDispatchDebugMessages);
  }
}

py::object CDebug::GetDebugContext(void)
{
  v8::HandleScope handle_scope;

  return py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(
    CContextPtr(new CContext(v8::Debug::GetDebugContext())))));
}

void CDebug::Listen(const std::string& name, int port, bool wait_for_connection)
{
  v8::Debug::EnableAgent(name.c_str(), port, wait_for_connection);
}

void CDebug::SendCommand(const std::string& cmd)
{
  std::vector<uint16_t> buf(cmd.length()+1);

  for (size_t i=0; i<cmd.length(); i++)
  {
    buf[i] = cmd[i];
  }

  buf[cmd.length()] = 0;

  v8::Debug::SendCommand(&buf[0], buf.size()-1);
}

void CDebug::OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
  v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data)
{
  v8::HandleScope scope;  
  
  CDebug *pThis = static_cast<CDebug *>(v8::Handle<v8::External>::Cast(data)->Value());

  if (!pThis->m_enabled) return;

  if (pThis->m_onDebugEvent.ptr() == Py_None) return;

  v8::Context::Scope context_scope(pThis->m_debug_context);  

  CJavascriptObjectPtr event_obj(new CJavascriptObject(event_data));

  CPythonGIL python_gil;

  py::call<void>(pThis->m_onDebugEvent.ptr(), event, event_obj);
}

class DebugClientData : public v8::Debug::ClientData
{
  py::object m_data;
public:
  DebugClientData(py::object data) : m_data(data) {}

  py::object data(void) const { return m_data; }
};

void CDebug::OnDebugMessage(const uint16_t* message, int length, v8::Debug::ClientData* client_data)
{
  if (GetInstance().m_onDebugMessage.ptr() == Py_None) return;

  v8::HandleScope scope;  

  v8::Handle<v8::String> msg(v8::String::New(message, length));

  v8::String::Utf8Value str(msg);

  py::object data;

  if (client_data)
  {
    data = static_cast<DebugClientData *>(client_data)->data();
  }

  CPythonGIL python_gil;

  py::call<void>(GetInstance().m_onDebugMessage.ptr(), py::str(*str, str.length()), data);
}

void CDebug::OnDispatchDebugMessages(void)
{  
  v8::Context::Scope context_scope(GetInstance().m_debug_context);  

  if (GetInstance().m_onDispatchDebugMessages.ptr() == Py_None ||
      py::call<bool>(GetInstance().m_onDispatchDebugMessages.ptr()))
  {
    v8::Debug::ProcessDebugMessages(); 
  }  
}

void CDebug::DebugBreakForCommand(py::object data)
{
  v8::Debug::DebugBreakForCommand(data.ptr() == Py_None ? NULL : new DebugClientData(data)); 
}

void CDebug::Expose(void)
{
  py::class_<CDebug, boost::noncopyable>("JSDebug", py::no_init)
    .add_property("enabled", &CDebug::IsEnabled, &CDebug::SetEnable)
    .add_property("context", &CDebug::GetDebugContext) 

    .def("debugBreak", &CDebug::DebugBreak)
    .def("debugBreakForCommand", &CDebug::DebugBreakForCommand)
    .def("sendCommand", &CDebug::SendCommand, (py::arg("command")))
    .def("loop", &CDebug::ProcessDebugMessages)  
    .def("listen", &CDebug::Listen, (py::arg("name"),
                                     py::arg("port"),
                                     py::arg("wait_for_connection") = false))

    .def_readwrite("onDebugEvent", &CDebug::m_onDebugEvent)
    .def_readwrite("onDebugMessage", &CDebug::m_onDebugMessage)
    .def_readwrite("onDispatchDebugMessages", &CDebug::m_onDispatchDebugMessages)
    ;

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
