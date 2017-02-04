#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import os
import platform

from distutils.util import strtobool

is_py3k = sys.version_info[0] > 2

is_winnt = os.name == "nt"
is_linux = os.name == "posix" and sys.platform.startswith("linux")
is_freebsd = os.name == "posix" and sys.platform.startswith("freebsd")
is_mac = os.name == "mac"
is_osx = os.name == "posix" and sys.platform == "darwin"
is_cygwin = os.name == "posix" and sys.platform == "cygwin"
is_mingw = is_winnt and os.environ.get('MSYSTEM', '').startswith('MINGW')
is_64bit = platform.architecture()[0] == "64bit"
is_arm = False

# default settings, you can modify it in buildconf.py.
# please look in buildconf.py.example for more information
PYV8_HOME = os.path.abspath(os.path.dirname(__file__))
BOOST_HOME = None
BOOST_MT = is_osx
BOOST_DEBUG = False
BOOST_STATIC_LINK = True
PYTHON_HOME = None
V8_HOME = None
V8_GIT_URL = "https://chromium.googlesource.com/v8/v8.git"
V8_GIT_TAG = "5.8.110"  # https://chromium.googlesource.com/v8/v8.git/+/5.8.110
DEPOT_HOME = None
DEPOT_GIT_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"
DEPOT_DOWNLOAD_URL = "https://storage.googleapis.com/chrome-infra/depot_tools.zip"

INCLUDE = None
LIB = None
OFFLINE_MODE = False
PYV8_DEBUG = False

MAKE = 'gmake' if is_freebsd else 'make'

# load defaults from config file
try:
    from buildconf import *
except ImportError:
    pass

# override defaults from environment
PYV8_HOME = os.environ.get('PYV8_HOME', PYV8_HOME)
BOOST_HOME = os.environ.get('BOOST_HOME', BOOST_HOME)
BOOST_MT = os.environ.get('BOOST_MT', BOOST_MT)
PYTHON_HOME = os.environ.get('PYTHON_HOME', PYTHON_HOME)
V8_HOME = os.environ.get('V8_HOME', V8_HOME)
V8_GIT_URL = os.environ.get('V8_GIT_URL', V8_GIT_URL)
DEPOT_HOME = os.environ.get('DEPOT_HOME', DEPOT_HOME)
INCLUDE = os.environ.get('INCLUDE', INCLUDE)
LIB = os.environ.get('LIB', LIB)
OFFLINE_MODE = bool(strtobool(os.environ.get('OFFLINE_MODE', str(OFFLINE_MODE))))
PYV8_DEBUG = bool(strtobool(os.environ.get('PYV8_DEBUG', str(PYV8_DEBUG))))
MAKE = os.environ.get('MAKE', MAKE)

if V8_HOME is None or not os.path.exists(os.path.join(V8_HOME, 'include', 'v8.h')):
    V8_HOME = os.path.join(PYV8_HOME, 'build', 'v8-' + V8_GIT_TAG)

if DEPOT_HOME is None:
    DEPOT_HOME = os.path.join(PYV8_HOME, 'depot_tools')

if type(PYV8_DEBUG) == str:
    PYV8_DEBUG = bool(strtobool(PYV8_DEBUG))

V8_BACKTRACE = True                     # Support for backtrace_symbols on linux.
V8_DISASSEMBLER = PYV8_DEBUG            # enable the disassembler to inspect generated code
V8_GDB_JIT = PYV8_DEBUG
V8_HANDLE_ZAPPING = True
V8_I18N = True
V8_INSPECTOR = True
V8_OBJECT_PRINT = PYV8_DEBUG
V8_SLOW_DCHECKS = PYV8_DEBUG
V8_TRACE_MAPS = PYV8_DEBUG
V8_EXTRA_CHECKS = PYV8_DEBUG
V8_VERIFY_CSA = PYV8_DEBUG
V8_VERIFY_HEAP = PYV8_DEBUG
V8_VERIFY_PREDICTABLE = PYV8_DEBUG
V8_NATIVE_REGEXP = True                 # Whether to use native or interpreted regexp implementation
V8_NO_INLINE = False
V8_USE_SNAPSHOT = True                  # build using snapshots for faster start-up
V8_DEBUG_SYMBOLS = True
V8_AST = False
V8_DEBUGGER = False

macros = [
    ("BOOST_PYTHON_STATIC_LIB", None),
]

if V8_DEBUGGER:
    macros += [('SUPPORT_DEBUGGER', None)]

if V8_I18N:
    macros += [('V8_I18N_SUPPORT', None)]

if V8_USE_SNAPSHOT:
    macros += [('V8_USE_EXTERNAL_STARTUP_DATA', None)]

boost_libs = [
    'boost_date_time',
    'boost_filesystem',
    'boost_log',
    'boost_log_setup',
    'boost_python',
    'boost_regex',
    'boost_system',
    'boost_thread',
]

if BOOST_MT:
    boost_libs = [lib + '-mt' for lib in boost_libs]

if BOOST_DEBUG:
    boost_libs = [lib + '-d' for lib in boost_libs]

include_dirs = [
    os.path.join(V8_HOME, 'include'),
    V8_HOME,
    os.path.join(V8_HOME, 'src'),
]

library_dirs = []
libraries = []
extra_compile_args = ['-std=c++11']
extra_link_args = []
extra_objects = []

if V8_AST:
    extra_compile_args = ['-DSUPPORT_AST=1']

if INCLUDE:
    include_dirs += [p for p in INCLUDE.split(os.path.pathsep) if p]
if LIB:
    library_dirs += [p for p in LIB.split(os.path.pathsep) if p]

if not is_winnt:
    include_dirs += ['/usr/local/include']
    library_dirs += ['/usr/local/lib']

    if os.path.isdir('/opt/local/'):
        include_dirs += ['/opt/local/include']
        library_dirs += ['/opt/local/lib']

    if BOOST_HOME:
        boost_lib_dir = os.path.join(BOOST_HOME, 'stage/lib')

        include_dirs += [BOOST_HOME]
        library_dirs += [boost_lib_dir]
    else:
        boost_lib_dir = '/usr/local/lib'

    if BOOST_STATIC_LINK:
        extra_link_args += [os.path.join(boost_lib_dir, "lib%s.a") % lib for lib in boost_libs]
    else:
        libraries += boost_libs

if is_winnt:
    include_dirs += [
        BOOST_HOME,
        os.path.join(PYTHON_HOME, 'include'),
    ]
    library_dirs += [
        os.path.join(BOOST_HOME, 'stage/lib'),
        os.path.join(BOOST_HOME, 'lib'),
        os.path.join(PYTHON_HOME, 'libs'),
    ]

    macros += [
        ("V8_TARGET_ARCH_X64" if is_64bit else "V8_TARGET_ARCH_IA32", None),
        ("WIN32", None),
    ]

    if not is_64bit:
        macros.append(("_USE_32BIT_TIME_T", None),)

    libraries += ["winmm", "ws2_32"]

    if PYV8_DEBUG:
        extra_compile_args += ["/Od", "/MTd", "/EHsc", "/Gy", "/Zi"]
    else:
        extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]

    extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X64" if is_64bit else "/MACHINE:X86"]

    if PYV8_DEBUG:
        extra_link_args += ["/PYV8_DEBUG"]

    os.putenv('MSSdk', 'true')
    os.putenv('DISTUTILS_USE_SDK', 'true')
elif is_linux or is_freebsd:
    if PYTHON_HOME:
        major, minor, _, _, _ = sys.version_info
        library_dirs += [os.path.join(PYTHON_HOME, 'lib/python%d.%d' % (major, minor))]
        include_dirs += [os.path.join(PYTHON_HOME, 'include')]

    extra_compile_args += ["-Wno-write-strings"]

    if is_freebsd:
        libraries += ["execinfo"]
        V8_STRICTALIASING = False

    libraries += ["rt"]

    if hasattr(os, 'uname'):
        if os.uname()[-1] in ('x86_64', 'amd64'):
            is_64bit = True
            extra_link_args += ["-fPIC"]
            macros += [("V8_TARGET_ARCH_X64", None)]
        elif os.uname()[-1].startswith('arm'):
            is_arm = True
            macros += [("V8_TARGET_ARCH_ARM", None)]
    else:
        macros += [("V8_TARGET_ARCH_IA32", None)]

    if is_linux:
        extra_link_args += ["-lrt"] # make ubuntu happy

    if PYV8_DEBUG:
        extra_compile_args += ['-g', '-O0', '-fno-inline']
    else:
        extra_compile_args += ['-g', '-O3']

elif is_mac: # contribute by Artur Ventura
    library_dirs += ['/lib']
    libraries += ["c"]

elif is_osx: # contribute by progrium and alec
    # force x64 because Snow Leopard's native Python is 64-bit
    # scons arch=x64 library=static

    if is_64bit:
        os.environ['ARCHFLAGS'] = '-arch x86_64'
        extra_link_args += ["-fPIC"]
        macros += [("V8_TARGET_ARCH_X64", None)]
    else:
        os.environ['ARCHFLAGS'] = '-arch i386'
        macros += [("V8_TARGET_ARCH_IA32", None)]

    if PYV8_DEBUG:
        extra_compile_args += ['-g', '-O0', '-fno-inline']
    else:
        extra_compile_args += ['-g', '-O3']

    extra_compile_args += ["-Wdeprecated-writable-strings", "-stdlib=libc++"]

else:
    print("ERROR: unsupported OS (%s) and platform (%s)" % (os.name, sys.platform))

arch = 'x64' if is_64bit else 'arm' if is_arm else 'ia32'
mode = 'debug' if PYV8_DEBUG else 'release'
target = arch + '.' + mode

v8_libs = ['v8', 'v8_libbase', 'v8_libplatform']

if V8_I18N:
    v8_libs += ['icuuc', 'icui18n']

v8_library_path = "%s/out.gn/%s/" % (V8_HOME, target)

libraries += v8_libs
library_dirs.append(v8_library_path)
