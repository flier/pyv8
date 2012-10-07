#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement

import sys, os, os.path, math, platform
import subprocess

is_winnt = os.name == "nt"
is_linux = os.name == "posix" and sys.platform.startswith("linux")
is_freebsd = os.name == "posix" and sys.platform.startswith("freebsd")
is_mac = os.name == "mac"
is_osx = os.name == "posix" and sys.platform == "darwin"
is_cygwin = os.name == "posix" and sys.platform == "cygwin"
is_mingw = is_winnt and os.environ.get('MSYSTEM').startswith('MINGW')

if is_cygwin or is_mingw:
    print "ERROR: Cygwin or MingGW is not official support, please try to use Visual Studio 2010 Express or later."
    sys.exit(-1)

import ez_setup
ez_setup.use_setuptools()

from distutils.command.build import build as _build
from setuptools import setup, Extension

# default settings, you can modify it in buildconf.py.
# please look in buildconf.py.example for more information
PYV8_HOME = os.path.abspath(os.path.dirname(__file__))
BOOST_HOME = None
BOOST_PYTHON_MT = False
BOOST_STATIC_LINK = False
PYTHON_HOME = None
V8_HOME = None
V8_SVN_URL = "http://v8.googlecode.com/svn/trunk/"
V8_SVN_REVISION = None

INCLUDE = None
LIB = None
DEBUG = False

V8_SNAPSHOT_ENABLED = True      # build using snapshots for faster start-up
V8_NATIVE_REGEXP = True         # Whether to use native or interpreted regexp implementation
V8_VMSTATE_TRACKING = DEBUG     # enable VM state tracking
V8_OBJECT_PRINT = DEBUG         # enable object printing
V8_PROTECT_HEAP = DEBUG         # enable heap protection
V8_DISASSEMBLEER = DEBUG        # enable the disassembler to inspect generated code
V8_DEBUGGER_SUPPORT = True      # enable debugging of JavaScript code
V8_PROFILING_SUPPORT = True     # enable profiling of JavaScript code
V8_INSPECTOR_SUPPORT = DEBUG    # enable inspector features
V8_LIVE_OBJECT_LIST = DEBUG     # enable live object list features in the debugger
V8_FAST_TLS = True              # enable fast thread local storage support

# load defaults from config file
try:
    from buildconf import *
except ImportError:
    pass

# override defaults from environment
PYV8_HOME = os.environ.get('PYV8_HOME', PYV8_HOME)
BOOST_HOME = os.environ.get('BOOST_HOME', BOOST_HOME)
BOOST_PYTHON_MT = os.environ.get('BOOST_PYTHON_MT', BOOST_PYTHON_MT)
PYTHON_HOME = os.environ.get('PYTHON_HOME', PYTHON_HOME)
V8_HOME = os.environ.get('V8_HOME', V8_HOME)
V8_SVN_URL = os.environ.get('V8_SVN_URL', V8_SVN_URL)
INCLUDE = os.environ.get('INCLUDE', INCLUDE)
LIB = os.environ.get('LIB', LIB)
DEBUG = os.environ.get('DEBUG', DEBUG)

if type(DEBUG) == str:
    DEBUG = DEBUG.lower() in ['true', 'on', 't']

if V8_HOME is None or not os.path.exists(os.path.join(V8_HOME, 'include', 'v8.h')):
    print "WARN: V8_HOME doesn't exists or point to a wrong folder, ",
    print "we will try to checkout and build a private build from <%s>." % V8_SVN_URL

    V8_HOME = os.path.join(PYV8_HOME, 'build', ('v8_r%d' % V8_SVN_REVISION) if V8_SVN_REVISION else 'v8')
else:
    svn_name = ('SVN r%d' % V8_SVN_REVISION) if V8_SVN_REVISION else 'the latest SVN trunk'

    print "Found Google v8 base on V8_HOME <%s>, update it to %s at <%s>" % (V8_HOME, svn_name, V8_SVN_URL)

source_files = ["Utils.cpp", "Exception.cpp", "Context.cpp", "Engine.cpp", "Wrapper.cpp",
                "Debug.cpp", "Locker.cpp", "AST.cpp", "PrettyPrinter.cpp", "PyV8.cpp"]

macros = [
    ("BOOST_PYTHON_STATIC_LIB", None),
]

if DEBUG:
    macros += [
        ("V8_ENABLE_CHECKS", None),
    ]

if V8_NATIVE_REGEXP:
    macros += [("V8_NATIVE_REGEXP", None)]
else:
    macros += [("V8_INTERPRETED_REGEXP", None)]

if V8_DISASSEMBLEER:
    macros += [("ENABLE_DISASSEMBLER", None)]

if V8_PROFILING_SUPPORT:
    V8_VMSTATE_TRACKING = True
    macros += [("ENABLE_LOGGING_AND_PROFILING", None)]

if V8_PROTECT_HEAP:
    V8_VMSTATE_TRACKING = True
    macros += [("ENABLE_HEAP_PROTECTION", None)]

if V8_VMSTATE_TRACKING:
    macros += [("ENABLE_VMSTATE_TRACKING", None)]

if V8_LIVE_OBJECT_LIST:
    V8_OBJECT_PRINT = True
    V8_DEBUGGER_SUPPORT = True
    V8_INSPECTOR_SUPPORT = True
    macros += [("LIVE_OBJECT_LIST", None)]

if V8_OBJECT_PRINT:
    macros += [("OBJECT_PRINT", None)]

if V8_DEBUGGER_SUPPORT:
    macros += [("ENABLE_DEBUGGER_SUPPORT", None)]

if V8_INSPECTOR_SUPPORT:
    macros += [("INSPECTOR", None)]

if V8_FAST_TLS:
    macros += [("V8_FAST_TLS", None)]

include_dirs = [
    os.path.join(V8_HOME, 'include'),
    V8_HOME,
    os.path.join(V8_HOME, 'src'),
]
library_dirs = []
libraries = []
extra_compile_args = []
extra_link_args = []

if INCLUDE:
    include_dirs += [p for p in INCLUDE.split(os.path.pathsep) if p]
if LIB:
    library_dirs += [p for p in LIB.split(os.path.pathsep) if p]

v8_lib = 'v8_g' if DEBUG else 'v8' # contribute by gaussgss
boost_lib = 'boost_python-mt' if BOOST_PYTHON_MT else 'boost_python'

classifiers = [
    'Development Status :: 4 - Beta',
    'Environment :: Plugins',
    'Intended Audience :: Developers',
    'Intended Audience :: System Administrators',
    'License :: OSI Approved :: Apache Software License',
    'Natural Language :: English',
    'Operating System :: Microsoft :: Windows',
    'Operating System :: POSIX',
    'Programming Language :: C++',
    'Programming Language :: Python',
    'Topic :: Internet',
    'Topic :: Internet :: WWW/HTTP',
    'Topic :: Software Development',
    'Topic :: Software Development :: Libraries :: Python Modules',
    'Topic :: Utilities',
]

description = """
PyV8 is a python wrapper for Google V8 engine, it act as a bridge
between the Python and JavaScript objects, and support to hosting
Google's v8 engine in a python script.
"""

if is_winnt:
    import platform
    is_64bit = platform.architecture()[0] == "64bit"

    include_dirs += [
        BOOST_HOME,
        os.path.join(PYTHON_HOME, 'include'),
    ]
    library_dirs += [
        V8_HOME,
        os.path.join(BOOST_HOME, 'stage/lib'),
        os.path.join(PYTHON_HOME, 'libs'),
    ]

    macros += [
        ("V8_TARGET_ARCH_X64" if is_64bit else "V8_TARGET_ARCH_IA32", None),
        ("WIN32", None),
    ]

    if not is_64bit:
        macros.append(("_USE_32BIT_TIME_T", None),)

    libraries += ["winmm", "ws2_32"]

    extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]
    extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X64" if is_64bit else "/MACHINE:X86"]
elif is_linux or is_freebsd:
    library_dirs += [
        V8_HOME,
    ]
    if BOOST_HOME:
        boost_lib_dir = os.path.join(BOOST_HOME, 'stage/lib')
        include_dirs += [BOOST_HOME]
    else:
        boost_lib_dir = '/usr/local/lib'
        include_dirs += ['/usr/local/include']

    library_dirs += [boost_lib_dir]

    if PYTHON_HOME:
        major, minor, _, _, _ = sys.version_info
        library_dirs += [os.path.join(PYTHON_HOME, 'lib/python%d.%d' % (major, minor))]
        include_dirs += [os.path.join(PYTHON_HOME, 'include')]

    libraries += [v8_lib, "rt"]
    extra_compile_args += ["-Wno-write-strings"]

    if BOOST_STATIC_LINK:
        extra_link_args.append(os.path.join(boost_lib_dir, "lib%s.a" % boost_lib))
    else:
        libraries.append(boost_lib)

    if is_freebsd:
        libraries += ["execinfo"]

    if hasattr(os, 'uname'):
        if os.uname()[-1] in ('x86_64', 'amd64'):
            extra_link_args += ["-fPIC"]
            macros += [("V8_TARGET_ARCH_X64", None)]
        elif os.uname()[-1].startswith('arm'):
            macros += [("V8_TARGET_ARCH_ARM", None)]
    else:
        macros += [("V8_TARGET_ARCH_IA32", None)]

elif is_mac: # contribute by Artur Ventura
    include_dirs += [
        BOOST_HOME,
    ]
    library_dirs += [os.path.join('/lib')]
    libraries += [boost_lib, v8_lib, "c"]

elif is_osx: # contribute by progrium and alec
    # force x64 because Snow Leopard's native Python is 64-bit
    # scons arch=x64 library=static

    include_dirs += [
        "/opt/local/include", # MacPorts$ sudo port install boost
        "/usr/local/include", # HomeBrew$ brew install boost
    ]

    library_dirs += [
        V8_HOME,
    ]

    libraries += ["boost_python-mt", v8_lib]

    is_64bit = math.trunc(math.ceil(math.log(sys.maxint, 2)) + 1) == 64 # contribute by viy

    if is_64bit:
        os.environ['ARCHFLAGS'] = '-arch x86_64'
        extra_link_args += ["-fPIC"]
        macros += [("V8_TARGET_ARCH_X64", None)]
    else:
        os.environ['ARCHFLAGS'] = '-arch i386'
        macros += [("V8_TARGET_ARCH_IA32", None)]

else:
    print "ERROR: unsupported OS (%s) and platform (%s)" % (os.name, sys.platform)

class build(_build):
    def checkout_v8(self):
        update_code = os.path.isdir(V8_HOME) and os.path.exists(os.path.join(V8_HOME, 'include', 'v8.h'))

        try:
            from pysvn import Client, Revision, opt_revision_kind

            svnClient = Client()
            rev = Revision(opt_revision_kind.number, V8_SVN_REVISION) if V8_SVN_REVISION else Revision(opt_revision_kind.head)

            if update_code:
                r = svnClient.update(V8_HOME, revision=rev)
            else:
                r = svnClient.checkout(V8_SVN_URL, V8_HOME, revision=rev)

            if r: return

            print "ERROR: Failed to export from V8 svn repository"            
        except ImportError:
            #print "WARN: could not import pysvn. Ensure you have the pysvn package installed."
            #print "      on debian/ubuntu, this is called 'python-svn'; on Fedora/RHEL, this is called 'pysvn'."

            print "INFO: we will try to use the system 'svn' command to checkout/update V8 code"

        if update_code:
            args = ["svn", "up", V8_HOME]
        else:
            os.makedirs(V8_HOME)

            args = ["svn", "co", V8_SVN_URL, V8_HOME]

        if V8_SVN_REVISION:
            args += ['-r', str(V8_SVN_REVISION)]

        try:
            proc = subprocess.Popen(args, stdout=sys.stdout, stderr=sys.stderr)

            proc.communicate()

            if proc.returncode != 0:
                print "WARN: fail to checkout or update Google v8 code from SVN, error code: ", proc.returncode
        except Exception, e:
            print "ERROR: fail to invoke 'svn' command, please install it first: %s" % e
            sys.exit(-1)

    def check_scons(self):
        # We'll have to compile using `scons': do we have scons installed?
        try:
            p = subprocess.Popen('scons --version', shell=True,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p.communicate()

            if out and 'Knight' in out:
                print "INFO: scons is installed; we can at least attempt to build v8"
            else:
                print "ERROR: scons doesn't appear to be installed or work correctly: %s" % err
                sys.exit(-1)
        except Exception, e:
            print "ERROR: scons doesn't appear to be installed: %s" % e
            print "       You must have the 'scons' package installed to build v8"
            print "       (called 'scons' on debian/ubuntu/fedora/RHEL)"
            sys.exit(-1)

    def build_v8(self):
        # Next up, we have to patch the SConstruct file from the v8 source to remove -no-rtti and -no-exceptions
        scons = os.path.join(V8_HOME, "SConstruct")

        # Check if we need to patch by searching for rtti flag in the data 
        with open(scons, 'r') as f:
            build_script = f.read()

        is_x64 = [k for k, v in macros if k == 'V8_TARGET_ARCH_X64']
        is_arm = [k for k, v in macros if k == 'V8_TARGET_ARCH_ARM']
        
        fixed_build_script = build_script.replace('-fno-rtti', '') \
                                         .replace('-fno-exceptions', '') \
                                         .replace('-Werror', '') \
                                         .replace('/WX', '').replace('/GR-', '')

        if is_x64 and os.name != 'nt':
            fixed_build_script = fixed_build_script.replace("['$DIALECTFLAGS', '$WARNINGFLAGS']", "['$DIALECTFLAGS', '$WARNINGFLAGS', '-fPIC']")

        if build_script == fixed_build_script:
            print "INFO: skip to patch the Google v8 SConstruct file "
        else:
            print "INFO: patch the Google v8 SConstruct file to enable RTTI and C++ Exceptions"

            if os.path.exists(scons + '.bak'):
                os.remove(scons + '.bak')

            os.rename(scons, scons + '.bak')

            with open(scons, 'w') as f:
                f.write(fixed_build_script)

        options = {
            'mode': 'debug' if DEBUG else 'release',
            'arch': 'x64' if is_x64 else 'arm' if is_arm else 'ia32',
            'regexp': 'native' if V8_NATIVE_REGEXP else 'interpreted',
            'snapshot': 'on' if V8_SNAPSHOT_ENABLED else 'off',
            'vmstate': 'on' if V8_VMSTATE_TRACKING else 'off',
            'objectprint': 'on' if V8_OBJECT_PRINT else 'off',
            'protectheap': 'on' if V8_PROTECT_HEAP else 'off',
            'profilingsupport': 'on' if V8_PROFILING_SUPPORT else 'off',
            'debuggersupport': 'on' if V8_DEBUGGER_SUPPORT else 'off',
            'inspector': 'on' if V8_INSPECTOR_SUPPORT else 'off',
            'liveobjectlist': 'on' if V8_LIVE_OBJECT_LIST else 'off',
            'disassembler': 'on' if V8_DISASSEMBLEER else 'off',
            'fasttls': 'on' if V8_FAST_TLS else 'off',
        }

        print "INFO: building Google v8 with SCons for %s platform" % options['arch']

        cmdline = "scons %s" % ' '.join(["%s=%s" % (k, v) for k, v in options.items()])

        print "DEBUG: building", cmdline

        proc = subprocess.Popen(cmdline, cwd=V8_HOME, shell=True, stdout=sys.stdout, stderr=sys.stderr)

        proc.communicate()

        if proc.returncode != 0:
            print "WARN: fail to build Google v8 code from SVN, error code: ", proc.returncode

    def run(self):
        try:
            self.checkout_v8()
            self.check_scons()
            self.build_v8()
        except Exception, e:
            print "ERROR: fail to checkout and build v8, %s" % e

        _build.run(self)

pyv8 = Extension(name = "_PyV8",
                 sources = [os.path.join("src", file) for file in source_files],
                 define_macros = macros,
                 include_dirs = include_dirs,
                 library_dirs = library_dirs,
                 libraries = libraries,
                 extra_compile_args = extra_compile_args,
                 extra_link_args = extra_link_args,
                 )

setup(name='PyV8',
      cmdclass = { 'build': build },
      version='1.0',
      description='Python Wrapper for Google V8 Engine',
      long_description=description,
      platforms="x86",
      author='Flier Lu',
      author_email='flier.lu@gmail.com',
      url='http://code.google.com/p/pyv8/',
      download_url='http://code.google.com/p/pyv8/downloads/list',
      license="Apache Software License",
      py_modules=['PyV8'],
      ext_modules=[pyv8],
      test_suite='PyV8',
      classifiers=classifiers)
