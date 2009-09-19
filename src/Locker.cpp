#include "Locker.h"

void CLocker::Expose(void)
{
  py::class_<CLocker, boost::noncopyable>("JSLocker")
    .add_static_property("actived", &v8::Locker::IsActive, 
                         "whether Locker is being used by this V8 instance.")
    .add_static_property("locked", &v8::Locker::IsLocked,
                         "whether or not the locker is locked by the current thread.")

    .def("StartPreemption", &v8::Locker::StartPreemption)    
    .def("StopPreemption", &v8::Locker::StopPreemption)
    .staticmethod("StartPreemption")
    .staticmethod("StopPreemption")

    .def("entered", &CLocker::entered)

    .def("enter", &CLocker::enter)
    .def("leave", &CLocker::leave)    
    ;

  py::class_<CUnlocker, boost::noncopyable>("JSUnlocker")
    .def("entered", &CUnlocker::entered)

    .def("enter", &CUnlocker::enter)
    .def("leave", &CUnlocker::leave)
    ;
}
