#pragma once

#include "Exception.h"

class CLocker
{
  std::auto_ptr<v8::Locker> m_locker;
public:  
  bool entered(void) { return m_locker.get(); }

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

  static void Expose(void);
};

class CUnlocker
{
  std::auto_ptr<v8::Unlocker> m_unlocker;
public:
  bool entered(void) { return m_unlocker.get(); }

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