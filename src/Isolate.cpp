#include "Isolate.h"

void CManagedIsolate::Expose(void)
{
    py::class_<CManagedIsolate, boost::noncopyable>("JSIsolate", py::no_init)
        .def(py::init<>("Creates a new isolate.  Does not change the currently entered isolate."))

        .add_property("locked", &CManagedIsolate::IsLocked)
        .add_property("used", &CManagedIsolate::InUse, "Check if this isolate is in use.")

        .def("enter", &CManagedIsolate::Enter,
             "Sets this isolate as the entered one for the current thread. "
             "Saves the previously entered one (if any), so that it can be "
             "restored when exiting.  Re-entering an isolate is allowed.")

        .def("leave", &CManagedIsolate::Leave,
             "Exits this isolate by restoring the previously entered one in the current thread. "
             "The isolate may still stay the same, if it was entered more than once.")

        .add_static_property("current", &CManagedIsolate::GetCurrent,
                             "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

        .def("GetCurrentStackTrace", &CManagedIsolate::GetCurrentStackTrace);
}

logger_t &CIsolateBase::Logger(void)
{
    auto logger = GetData<logger_t>(DataSlots::LoggerIndex, [this]() {
        std::auto_ptr<logger_t> logger(new logger_t());

        logger->add_attribute(ISOLATE_ATTR, attrs::constant<const v8::Isolate *>(m_isolate));

        return logger.release();
    });

    return *logger;
}

CIsolate::CIsolate(v8::Isolate *isolate) : CIsolateBase(isolate)
{
    BOOST_LOG_SEV(Logger(), trace) << "isolated wrapped";
}

v8::Local<v8::ObjectTemplate> CIsolate::ObjectTemplate(void)
{
    auto object_template = GetData<v8::Persistent<v8::ObjectTemplate>>(DataSlots::ObjectTemplateIndex, [this]() {
        v8::HandleScope handle_scope(m_isolate);

        return new v8::Persistent<v8::ObjectTemplate>(m_isolate, CPythonObject::CreateObjectTemplate(m_isolate));
    });

    return object_template->Get(m_isolate);
}

CManagedIsolate::CManagedIsolate() : CIsolateBase(CreateIsolate())
{
    BOOST_LOG_SEV(Logger(), trace) << "isolated created";
}

CManagedIsolate::~CManagedIsolate(void)
{
    BOOST_LOG_SEV(Logger(), trace) << "isolated destroyed";

    ClearDataSlots();

    m_isolate->Dispose();
}

void CManagedIsolate::ClearDataSlots() const
{
    delete GetData<logger_t>(DataSlots::LoggerIndex);
    delete GetData<v8::Persistent<v8::ObjectTemplate>>(DataSlots::ObjectTemplateIndex);
}

v8::Isolate *CManagedIsolate::CreateIsolate()
{
    v8::Isolate::CreateParams params;

    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    return v8::Isolate::New(params);
}

py::object CManagedIsolate::GetCurrent(void)
{
    v8::Isolate *isolate = v8::Isolate::GetCurrent();

    v8::HandleScope handle_scope(isolate);

    return !isolate ? py::object() : py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CIsolate>(
                                         CIsolatePtr(new CIsolate(isolate)))));
}
