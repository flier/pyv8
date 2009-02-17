#include "Exception.h"

void CPythonException::Expose(void)
{
  py::register_exception_translator<CEngineException>(CPythonException::translator);
  py::register_exception_translator<CWrapperException>(CPythonException::translator);
}
