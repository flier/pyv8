.. _build:

Build and Install
=================

Build Steps
-----------
PyV8 is using the setuptools [#f1]_, which is a famous building system for the Python.
You could use it to build the PyV8 module or distribution with one command.

Precompiled Packages
^^^^^^^^^^^^^^^^^^^^
If you want to try pyv8 ASAP, we suggest you use the precompiled package for your platform.

Please downloads the precompiled `packages <http://code.google.com/p/pyv8/downloads/list>`_ for Windows and Linux/Debian.

Build the module
^^^^^^^^^^^^^^^^
Run the setup.py with install or build command

.. code-block:: sh

    $python setup.py install     # build and install the PyV8
    $python setup.py build       # build the PyV8 package

Build the distribution
^^^^^^^^^^^^^^^^^^^^^^
Run the setup.py with bdist or bdist_wininst command

.. code-block:: sh

    $python setup.py bdist           # build the distribution package for Linux/Mac
    $python setup.py bdist_wininst   # build the distribution package for Windows

Third Party Library
-------------------

Python
^^^^^^

Please download and install Python 2.5 or later first.

Set environment variable PYTHON_HOME to the python root folder

.. code-block:: guess

    C:\>set PYTHON_HOME=C:\Program Files\Python

Boost
^^^^^

PyV8 use `boost.python <http://www.boost.org/doc/libs/release/libs/python/doc/>`_ [#f2]_ for interoperability,
please download or install `the latest version <http://www.boost.org/users/download/>`_ of Boost,
and follow `the getting started guide <http://www.boost.org/doc/libs/release/more/getting_started/>`_ to build the library.

Set environment variable BOOST_HOME to the boost root folder

.. code-block:: sh

    $export BOOST_HOME=~/boost_1_47_0

Google V8
^^^^^^^^^

Please follow `the building document <http://code.google.com/apis/v8/build.html>`_ to download and build v8 engine as static library

set environment variable V8_HOME to the v8 root folder

.. code-block:: sh

    $export V8_HOME=~/v8

If PyV8's setup.py can't found the v8 source from V8_HOME, it will automatic try to checkout the latest v8 SNV trunk code and build it.

.. note::

    If you want to build v8 with GCC 4.x at x64 platform, you should compile v8 with PIC (Position-Independent Code) mode [#f3]_,and set the arch to x64 for scons.

    .. code-block:: sh

        $export CCFLAGS=-fPIC
        $scons arch=x64

    You may also need build Boost with '-fPIC' argument.

    .. code-block:: sh

        $./bjam --clean
        $./bjam cxxflags=-fPIC

.. note::

    From the v8 engine 3.0, the build script of v8 engine has disabled the C++ RTTI and exception supports by default. It may save some bits of memory, but will broke the boost::python library and PyV8.

    So, we must enable it before build PyV8.

    For the GCC and other environment base on the SCons, please remove the -fno-rtti and -fno-exceptions from SConstruct

    .. code-block:: diff

        Index: SConstruct
        ===================================================================
        --- SConstruct  (revision 6041)
        +++ SConstruct  (working copy)
        @@ -126,7 +126,7 @@
           'gcc': {
             'all': {
               'CCFLAGS':      ['$DIALECTFLAGS', '$WARNINGFLAGS'],
        -      'CXXFLAGS':     ['$CCFLAGS', '-fno-rtti', '-fno-exceptions'],
        +      'CXXFLAGS':     ['$CCFLAGS'],
             },
             'visibility:hidden': {
               # Use visibility=default to disable this.
        @@ -455,7 +455,6 @@
           'gcc': {
             'all': {
               'LIBPATH': ['.'],
        -      'CCFLAGS': ['-fno-rtti', '-fno-exceptions']
             },
             'os:linux': {
               'LIBS':         ['pthread'],

    If you are using PyV8's setup.py to checkout and build v8, it will automatic patch the SConstruct file before build v8.

Debug Mode
----------
You could build Google v8 and PyV8 with debug mode, for more detail debug information.

1. Build v8 with debug mode

.. code-block:: sh

    $scons mode=debug            # x86
    $scons mode=debug arch=x64   # amd64

2. Set or export DEBUG=1

.. code-block:: sh

    $export DEBUG=1 

3. Build pyv8 with setup.py

.. code-block:: sh

    $python setup.py build

FAQ
----

1. Fail to load PyV8.so with AttributeError exception

    If you got the exception "AttributeError: 'Boost.Python.StaticProperty' object attribute 'doc' is read-only", please check your Python version. The Python 2.6.4 introduce a known issue which will break boost 1.40 or earlier version. Please use Python 2.5.x/2.6.3, or upgrade your boost to 1.41 or later.

.. toctree::
   :maxdepth: 2

.. rubric:: Footnotes

.. [#f1] `setuptools <http://peak.telecommunity.com/DevCenter/setuptools>`_ is a collection of enhancements to the Python distutils (for Python 2.3.5 and up on most platforms; 64-bit platforms require a minimum of Python 2.4) that allow you to more easily build and distribute Python packages, especially ones that have dependencies on other packages.

.. [#f2] `Boost.Python <http://www.boost.org/doc/libs/release/libs/python/doc/>`_ is a C++ library which enables seamless interoperability between C++ and the Python programming language. The new version has been rewritten from the ground up, with a more convenient and flexible interface, and many new capabilities, including support for:

        * References and Pointers
        * Globally Registered Type Coercions
        * Automatic Cross-Module Type Conversions
        * Efficient Function Overloading
        * C++ to Python Exception Translation
        * Default Arguments
        * Keyword Arguments
        * Manipulating Python objects in C++
        * Exporting C++ Iterators as Python Iterators
        * Documentation Strings

.. [#f3] `Position-Independent Code (PIC) <http://en.wikipedia.org/wiki/Position-independent_code>`_  is machine instruction code that executes properly regardless of where in memory it resides. PIC is commonly used for shared libraries, so that the same library code can be loaded in a location in each program address space where it will not overlap any other uses of memory (for example, other shared libraries).