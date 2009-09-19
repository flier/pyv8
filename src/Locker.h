#pragma once

#include "Exception.h"

class CLocker
{
  std::auto_ptr<v8::Locker> m_locker;
public:  
  bool entered(void) { return m_locker.get(); }

  void enter(void) { m_locker.reset(new v8::Locker()); }
  void leave(void) { m_locker.reset(); }  

  static void Expose(void);
};

class CUnlocker
{
  std::auto_ptr<v8::Unlocker> m_unlocker;
public:
  bool entered(void) { return m_unlocker.get(); }

  void enter(void) { m_unlocker.reset(new v8::Unlocker()); }
  void leave(void) { m_unlocker.reset(); }
};