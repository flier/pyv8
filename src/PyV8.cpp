// PyV8.cpp : Defines the entry point for the DLL application.
//
#include "Engine.h"
#include "Debug.h"

BOOST_PYTHON_MODULE(_PyV8)
{
  CJavascriptObject::Expose();
  CEngine::Expose();
  CDebug::Expose();  
}
