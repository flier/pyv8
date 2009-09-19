// PyV8.cpp : Defines the entry point for the DLL application.
//
#include "Engine.h"
#include "Debug.h"
#include "Locker.h"

BOOST_PYTHON_MODULE(_PyV8)
{
  CJavascriptException::Expose();
  CWrapper::Expose(); 
  CContext::Expose();
  CEngine::Expose();
  CDebug::Expose();  
  CLocker::Expose();
}
