#!/usr/bin/env python
import os, os.path
from distutils.core import setup, Extension
import distutils.msvccompiler

source_files = ["Engine.cpp", "Wrapper.cpp", "PyV8.cpp"]

macros = [("BOOST_PYTHON_STATIC_LIB", None)]
third_party_libraries = ["python", "boost", "v8"]

include_dirs = os.environ["INCLUDE"].split(';') + [os.path.join("lib", lib, "inc") for lib in third_party_libraries]
library_dirs = os.environ["LIB"].split(';') + [os.path.join("lib", lib, "lib") for lib in third_party_libraries]
libraries = ["winmm"]

pyv8 = Extension(name = "_PyV8", 
                 sources = [os.path.join("src", file) for file in source_files],
                 define_macros = macros,
                 include_dirs = include_dirs,
                 library_dirs = library_dirs,
                 libraries = libraries,
                 extra_compile_args = ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"],
                 extra_link_args = ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X86"],
                 )

setup(name='PyV8',
      version='0.1',
      description='Python Wrapper for Google V8 Engine',
      author='Flier Lu',
      author_email='flier.lu@gmail.com',
      url='http://code.google.com/p/pyv8/',
      license="Apache 2.0",
      py_modules=['PyV8'],
      ext_modules=[pyv8]
      )