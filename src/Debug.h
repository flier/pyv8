#pragma once

#include <vector>

#include "Wrapper.h"
#include "Utils.h"

#include <v8-debug.h>

class CDebug
{
  bool m_enabled;

  py::object m_onDebugEvent;
  py::object m_onDebugMessage;
  py::object m_onDispatchDebugMessages;

  v8::Persistent<v8::Context> m_debug_context, m_eval_context;

  static void OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
    v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data);
  static void OnDebugMessage(const uint16_t* message, int length, 
    v8::Debug::ClientData* client_data);
  static void OnDispatchDebugMessages(void);

  void Init(void);
public:
  CDebug() : m_enabled(false)
  {
    Init();
  }

  bool IsEnabled(void) { return m_enabled; }
  void SetEnable(bool enable);

  void DebugBreak(void) { v8::Debug::DebugBreak(); }
  void DebugBreakForCommand(py::object data);
  void ProcessDebugMessages(void) { v8::Debug::ProcessDebugMessages(); }
  
  void Listen(const std::string& name, int port, bool wait_for_connection);
  void SendCommand(const std::string& cmd);

  py::object GetDebugContext(void);
  py::object GetEvalContext(void);

  static CDebug& GetInstance(void)
  {
    static CDebug s_instance;

    return s_instance;
  }

  static void Expose(void);
};
