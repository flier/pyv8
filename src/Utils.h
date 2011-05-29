#pragma once

#include <string>

#ifdef _WIN32

#ifdef DEBUG
# pragma warning( push )
#endif

#pragma warning( disable : 4100 ) // 'identifier' : unreferenced formal parameter
#pragma warning( disable : 4121 ) // 'symbol' : alignment of a member was sensitive to packing
#pragma warning( disable : 4127 ) // conditional expression is constant
#pragma warning( disable : 4189 ) // 'identifier' : local variable is initialized but not referenced
#pragma warning( disable : 4244 ) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning( disable : 4505 ) // 'function' : unreferenced local function has been removed
#pragma warning( disable : 4512 ) // 'class' : assignment operator could not be generated
#pragma warning( disable : 4800 ) // 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( disable : 4996 ) // 'function': was declared deprecated

#ifndef _USE_32BIT_TIME_T
# define _USE_32BIT_TIME_T
#endif

#else

#include <cmath>
using std::isnan;

#ifndef isfinite
# include <limits>
# define isfinite(val) (val <= std::numeric_limits<double>::max())
#endif

#include <strings.h>
#define strnicmp strncasecmp

#define _countof(array) (sizeof(array)/sizeof(array[0]))

#endif

#include <boost/python.hpp>
namespace py = boost::python;

#include <v8.h>

#ifdef _WIN32

#ifdef DEBUG
  #pragma comment( lib, "v8_g.lib" )
#else
  #pragma comment( lib, "v8.lib" )
#endif

#ifdef DEBUG
# pragma warning( pop )
#endif

#endif 

v8::Handle<v8::String> ToString(const std::string& str);
v8::Handle<v8::String> ToString(const std::wstring& str);
v8::Handle<v8::String> ToString(py::object str);

v8::Handle<v8::String> DecodeUtf8(const std::string& str);

struct CPythonGIL
{
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};
