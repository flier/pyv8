// PyV8.cpp : Defines the entry point for the DLL application.
//
#include "Engine.h"

BOOST_PYTHON_MODULE(_PyV8)
{
  CEngine::Expose();
  CScript::Expose();
}