#include "Isolate.h"

void CIsolate::Expose(void)
{
  py::class_<CIsolate, boost::noncopyable>("JSIsolate", "JSIsolate is an isolated instance of the V8 engine.", py::no_init)
    .def(py::init<bool>((py::arg("owned") = false)))

    .add_static_property("current", &CIsolate::GetCurrent,
                         "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

    .add_property("locked", &CIsolate::IsLocked)
    .add_property("used", &CIsolate::InUse,
                  "Check if this isolate is in use.")

    .def("GetCurrentStackTrace", &CIsolate::GetCurrentStackTrace)

    .def("enter", &CIsolate::Enter,
         "Sets this isolate as the entered one for the current thread. "
         "Saves the previously entered one (if any), so that it can be "
         "restored when exiting.  Re-entering an isolate is allowed.")

    .def("leave", &CIsolate::Leave,
         "Exits this isolate by restoring the previously entered one in the current thread. "
         "The isolate may still stay the same, if it was entered more than once.")
    ;
}

CIsolate::CIsolate(bool owned) : m_owned(owned)
{
    v8::Isolate::CreateParams params;

    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    m_isolate = v8::Isolate::New(params);

    initLogger();

    BOOST_LOG_SEV(m_logger, trace) << "isolated created";
}

CIsolate::CIsolate(v8::Isolate *isolate) : m_isolate(isolate), m_owned(false)
{
    initLogger();

    BOOST_LOG_SEV(m_logger, trace) << "isolated wrapped";
}

CIsolate::~CIsolate(void)
{
    if (m_owned)
    {
        ClearDataSlots();

        m_isolate->Dispose();

        BOOST_LOG_SEV(m_logger, trace) << "isolated destroyed";
    }
}

py::object CIsolate::GetCurrent(void)
{
  v8::Isolate *isolate = v8::Isolate::GetCurrent();

  v8::HandleScope handle_scope(isolate);

  return !isolate ? py::object() :
    py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CIsolate>(
    CIsolatePtr(new CIsolate(isolate)))));
}

v8::Local<v8::ObjectTemplate> CIsolate::GetObjectTemplate(void) {
    auto object_template = GetData<v8::ObjectTemplate>(CIsolate::Slots::ObjectTemplate);

    if (object_template == nullptr) {
        object_template = new v8::Persistent<v8::ObjectTemplate>(m_isolate, CPythonObject::CreateObjectTemplate(m_isolate));

        SetData(CIsolate::Slots::ObjectTemplate, object_template);
    }

    return object_template->Get(m_isolate);
}
