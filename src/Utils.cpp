#ifndef _WIN32
#include "Python.h"
#endif

#include "Utils.h"

#include <vector>
#include <iterator>

#include "utf8.h"
#include "Locker.h"
#include "V8Internal.h"

v8::Handle<v8::String> ToString(const std::string& str, v8::Isolate *isolate)
{
  v8::EscapableHandleScope escapable_handle_scope(isolate);

  auto maybe_str = v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal, str.size());

  return maybe_str.IsEmpty() ? v8::String::Empty(isolate) : escapable_handle_scope.Escape(maybe_str.ToLocalChecked());
}
v8::Handle<v8::String> ToString(const std::wstring& str, v8::Isolate *isolate)
{
  v8::EscapableHandleScope escapable_handle_scope(isolate);

  const std::string utf8_str = EncodeUtf8(str);

  auto maybe_str = v8::String::NewFromUtf8(isolate, utf8_str.c_str(), v8::NewStringType::kNormal, utf8_str.size());

  return maybe_str.IsEmpty() ? v8::String::Empty(isolate) : escapable_handle_scope.Escape(maybe_str.ToLocalChecked());
}
v8::Handle<v8::String> ToString(py::object str, v8::Isolate *isolate)
{
  v8::EscapableHandleScope escapable_handle_scope(isolate);

  if (PyBytes_CheckExact(str.ptr()))
  {
    auto maybe_str = v8::String::NewFromUtf8(isolate,
                                             PyBytes_AS_STRING(str.ptr()),
                                             v8::NewStringType::kNormal,
                                             PyBytes_GET_SIZE(str.ptr()));

    return maybe_str.IsEmpty() ? v8::String::Empty(isolate) : escapable_handle_scope.Escape(maybe_str.ToLocalChecked());
  }

  if (PyUnicode_CheckExact(str.ptr()))
  {
  #ifndef Py_UNICODE_WIDE
    auto maybe_str = v8::String::NewFromTwoByte(isolate,
                                                reinterpret_cast<const uint16_t *>(PyUnicode_AS_UNICODE(str.ptr())),
                                                v8::NewStringType::kNormal,
                                                PyUnicode_GET_SIZE(str.ptr()));
  #else
    Py_ssize_t len = PyUnicode_GET_SIZE(str.ptr());
    const uint32_t *p = reinterpret_cast<const uint32_t *>(PyUnicode_AS_UNICODE(str.ptr()));

    std::vector<uint16_t> data(len+1);

    for(Py_ssize_t i=0; i<len; i++)
    {
      data[i] = (uint16_t) (p[i]);
    }

    data[len] = 0;

    auto maybe_str = v8::String::NewFromTwoByte(isolate, &data[0], v8::NewStringType::kNormal, len);
  #endif
    return maybe_str.IsEmpty() ? v8::String::Empty(isolate) : escapable_handle_scope.Escape(maybe_str.ToLocalChecked());
  }

  return ToString(py::object(py::handle<>(::PyObject_Str(str.ptr()))));
}

v8::Handle<v8::String> DecodeUtf8(const std::string& str, v8::Isolate *isolate)
{
  v8::EscapableHandleScope escapable_handle_scope(isolate);

  auto maybe_str = v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal, str.size());

  return maybe_str.IsEmpty() ? v8::String::Empty(isolate) : escapable_handle_scope.Escape(maybe_str.ToLocalChecked());
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
  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL()
{
  ::PyGILState_Release(m_state);
}
