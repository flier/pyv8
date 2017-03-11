// PyV8.cpp : Defines the entry point for the DLL application.
//
#include "libplatform/libplatform.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <Python.h>
#include <frameobject.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "Config.h"
#include "Engine.h"
#include "Locker.h"
#include "Utils.h"

#ifdef SUPPORT_DEBUGGER
#include "Debug.h"
#endif

#ifdef SUPPORT_AST
#include "AST.h"
#endif

static void load_external_data(logger_t &logger)
{
  const char *filename = ::PyString_AsString(::PyThreadState_Get()->frame->f_code->co_filename);
  fs::path load_path = fs::absolute(fs::path(filename).parent_path() / "v8");

#ifdef V8_I18N_SUPPORT
  fs::path icu_data_path = load_path / "icudtl.dat";

  if (v8::V8::InitializeICUDefaultLocation(filename, icu_data_path.c_str()))
  {
    BOOST_LOG_SEV(logger, info) << "loaded ICU data from " << icu_data_path.c_str();
  }
  else
  {
    BOOST_LOG_SEV(logger, warning) << "fail to load ICU data from " << icu_data_path.c_str();
  }
#endif

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  fs::path startup_data_path = load_path / "*.bin";

  BOOST_LOG_SEV(logger, info) << "loading external snapshot from " << startup_data_path.c_str() << "...";

  v8::V8::InitializeExternalStartupData(startup_data_path.c_str());
#endif
}

BOOST_PYTHON_MODULE(_PyV8)
{
  initialize_logging();

  logger_t logger;

  load_external_data(logger);

  BOOST_LOG_SEV(logger, debug) << "initializing platform ...";

  v8::V8::InitializePlatform(v8::platform::CreateDefaultPlatform());

  BOOST_LOG_SEV(logger, debug) << "initializing V8 v" << v8::V8::GetVersion() << "...";

  v8::V8::Initialize();

  BOOST_LOG_SEV(logger, debug) << "entering default isolate ...";

  auto isolate = new CManagedIsolate();

  isolate->Enter();

  BOOST_LOG_SEV(logger, debug) << "exposing modules ...";

  CJavascriptException::Expose();
  CWrapper::Expose();
  CManagedIsolate::Expose();
  CContext::Expose();
  CEngine::Expose();
  CLocker::Expose();

#ifdef SUPPORT_DEBUGGER
  CDebug::Expose();
#endif

#ifdef SUPPORT_AST
  CAstNode::Expose();
#endif
}
