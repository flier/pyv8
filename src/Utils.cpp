#ifndef _WIN32
#include "Python.h"
#endif

#include "Utils.h"

#include <vector>
#include <iterator>

#include "utf8.h"
#include "Locker.h"
#include "V8Internal.h"

v8::Handle<v8::String> ToString(const std::string& str)
{
  v8::HandleScope scope;

  return scope.Close(v8::String::New(str.c_str(), str.size()));
}
v8::Handle<v8::String> ToString(const std::wstring& str)
{
  v8::HandleScope scope;

  if (sizeof(wchar_t) == sizeof(uint16_t))
  {
    return scope.Close(v8::String::New((const uint16_t *) str.c_str(), str.size()));
  }

  std::vector<uint16_t> data(str.size()+1);

  for (size_t i=0; i<str.size(); i++)
  {
    data[i] = (uint16_t) str[i];
  }

  data[str.size()] = 0;

  return scope.Close(v8::String::New(&data[0], str.size()));
}
v8::Handle<v8::String> ToString(py::object str)
{
  v8::HandleScope scope;

  if (PyString_CheckExact(str.ptr()))
  {
    return scope.Close(v8::String::New(PyString_AS_STRING(str.ptr()), PyString_GET_SIZE(str.ptr())));  
  }
  
  if (PyUnicode_CheckExact(str.ptr()))
  {
  #ifndef Py_UNICODE_WIDE
    return scope.Close(v8::String::New(reinterpret_cast<const uint16_t *>(PyUnicode_AS_UNICODE(str.ptr()))));

  #else
    Py_ssize_t len = PyUnicode_GET_SIZE(str.ptr());
    const uint32_t *p = reinterpret_cast<const uint32_t *>(PyUnicode_AS_UNICODE(str.ptr()));
    
    std::vector<uint16_t> data(len+1);

    for(Py_ssize_t i=0; i<len; i++)
    {
      data[i] = (uint16_t) (p[i]);
    }

    data[len] = 0;

    return scope.Close(v8::String::New(&data[0], len));
  #endif
  }
  
  return ToString(py::object(py::handle<>(::PyObject_Str(str.ptr()))));
}

v8::Handle<v8::String> DecodeUtf8(const std::string& str)
{
  v8::HandleScope scope;

  std::vector<uint16_t> data;

  try
  {
    utf8::utf8to16(str.begin(), str.end(), std::back_inserter(data));

    return scope.Close(v8::String::New(&data[0], data.size()));
  }
  catch (const std::exception&)
  {
  	return scope.Close(v8::String::New(str.c_str(), str.size()));
  }
}

const std::string EncodeUtf8(const std::wstring& str)
{
  std::vector<uint8_t> data;

  if (sizeof(wchar_t) == sizeof(uint16_t))
  {
    utf8::utf16to8(str.begin(), str.end(), std::back_inserter(data));
  }
  else
  {
    utf8::utf32to8(str.begin(), str.end(), std::back_inserter(data));
  }

  return std::string((const char *) &data[0], data.size());
}

CPythonGIL::CPythonGIL() 
{
  while (CLocker::IsPreemption() && _PyThreadState_Current && ::PyGILState_GetThisThreadState() != ::_PyThreadState_Current)
  {
    v8::internal::Thread::YieldCPU();
  }

  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL()
{
  ::PyGILState_Release(m_state);
}  
