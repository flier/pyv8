#pragma once

#include <vector>

#include <v8-debug.h>

#include "Wrapper.h"

class CDebug;

struct CDebugEvent
{
  std::vector<py::object> m_callbacks;

  void Register(py::object callback) { m_callbacks.push_back(callback); }
  void Unregister(py::object callback)
  {
    std::vector<py::object>::iterator it = 
      std::find(m_callbacks.begin(), m_callbacks.end(), callback);

    if (it != m_callbacks.end())
      m_callbacks.erase(it);
  }

  CDebugEvent& operator+=(py::object callback) { Register(callback); return *this; }
  CDebugEvent& operator-=(py::object callback) { Unregister(callback); return *this; }
};  

class CDebug
{
  bool m_enabled;

  v8::Persistent<v8::Context> m_global_context, m_evaluation_context;

  CDebugEvent m_onMessage, m_onBreak, m_onException, m_onNewFunction, 
              m_onBeforeCompile, m_onAfterCompile;

  const CDebugEvent& GetDebugEvent(v8::DebugEvent event);

  static void OnDebugEvent(v8::DebugEvent event, v8::Handle<v8::Object> exec_state, 
    v8::Handle<v8::Object> event_data, v8::Handle<v8::Value> data);
  static void OnDebugMessage(const uint16_t* message, int length, void* data);

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