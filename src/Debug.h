#pragma once

#include <vector>

#include <v8-debug.h>

#include "Wrapper.h"

class CDebug
{
  bool m_enabled;

  py::object m_onDebugEvent;
  static py::object s_onDebugMessage;

  v8::Persistent<v8::Context> m_global_context;

  static void OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
    v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data);
  static void OnDebugMessage(const uint16_t* message, int length, v8::Debug::ClientData* client_data);

  void Init(void);
public:
  CDebug() : m_enabled(false)
  {
    Init();
  }

  bool IsEnabled(void) { return m_enabled; }
  void SetEnable(bool enable);

  static CDebug& GetInstance(void)
  {
    static CDebug s_instance;

    return s_instance;
  }

  static void Expose(void);
};
