#pragma once

#include "Exception.h"
#include "Utils.h"

class CLocker
{
  static bool s_preemption;

  std::auto_ptr<v8::Locker> m_locker;
public:  
  bool entered(void) { return NULL != m_locker.get(); }

  void enter(void) 
  { 
    Py_BEGIN_ALLOW_THREADS

    m_locker.reset(new v8::Locker()); 

    Py_END_ALLOW_THREADS
  }
  void leave(void) 
  { 
    Py_BEGIN_ALLOW_THREADS

    m_locker.reset(); 

    Py_END_ALLOW_THREADS
  }  

  static bool IsPreemption(void) { return s_preemption; }
  static void StartPreemption(int every_n_ms);
  static void StopPreemption(void);

  static void Expose(void);
};

class CUnlocker
{
  std::auto_ptr<v8::Unlocker> m_unlocker;
public:
  bool entered(void) { return NULL != m_unlocker.get(); }

  void enter(void) 
  { 
    Py_BEGIN_ALLOW_THREADS

    m_unlocker.reset(new v8::Unlocker()); 

    Py_END_ALLOW_THREADS
  }
  void leave(void) 
  { 
    Py_BEGIN_ALLOW_THREADS

    m_unlocker.reset(); 

    Py_END_ALLOW_THREADS
  }
};