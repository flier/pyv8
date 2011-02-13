#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys, os, os.path, platform
import subprocess

import ez_setup
ez_setup.use_setuptools()

from distutils.command.build import build as _build
from setuptools import setup, Extension

# default settings, you can modify it in buildconf.py.
# please look in buildconf.py.example for more information
PYV8_HOME = os.path.dirname(__file__)
BOOST_HOME = None
BOOST_PYTHON_MT = False
PYTHON_HOME = None
V8_HOME = None
V8_SVN_URL = "http://v8.googlecode.com/svn/trunk/"
V8_SVN_REVISION = None
V8_SNAPSHOT_ENABLED = True
INCLUDE = None
LIB = None
DEBUG = False

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

source_files = ["Exception.cpp", "Context.cpp", "Engine.cpp", "Wrapper.cpp",
                "Debug.cpp", "Locker.cpp", "AST.cpp", "PrettyPrinter.cpp", "PyV8.cpp"]

macros = [
    ("BOOST_PYTHON_STATIC_LIB", None),
    ("V8_NATIVE_REGEXP", None),
    ("ENABLE_DISASSEMBLER", None),
    ("ENABLE_LOGGING_AND_PROFILING", None),
    ("ENABLE_DEBUGGER_SUPPORT", None),
]

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

if os.name == "nt":
    include_dirs += [
        BOOST_HOME,
        os.path.join(PYTHON_HOME, 'include'),
    ]
    library_dirs += [
        os.path.join(V8_HOME, 'tools\\visual_studio\\Release\\lib'),
        os.path.join(BOOST_HOME, 'stage/lib'),
        os.path.join(PYTHON_HOME, 'libs'),
    ]

    macros += [("V8_TARGET_ARCH_IA32", None), ("WIN32", None)]

    libraries += ["winmm", "ws2_32"]
    extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]
    extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X86"]

elif os.name == "posix" and sys.platform == "linux2":
    library_dirs += [
        V8_HOME,
    ]
    if BOOST_HOME:
        library_dirs += [os.path.join(BOOST_HOME, 'stage/lib')]
        include_dirs += [BOOST_HOME]

    if PYTHON_HOME:
        major, minor, _, _, _ = sys.version_info
        library_dirs += [os.path.join(PYTHON_HOME, 'lib/python%d.%d' % (major, minor))]
        include_dirs += [os.path.join(PYTHON_HOME, 'include')]

    libraries += ["boost_python-mt" if BOOST_PYTHON_MT else "boost_python", v8_lib, "rt"]
    extra_compile_args += ["-Wno-write-strings"]

    if hasattr(os, 'uname') and os.uname()[-1] == 'x86_64':
        extra_link_args += ["-fPIC"]
        macros += [("V8_TARGET_ARCH_X64", None)]
    else:
        macros += [("V8_TARGET_ARCH_IA32", None)]

elif os.name == "mac": # contribute by Artur Ventura
    include_dirs += [
        BOOST_HOME,
    ]
    library_dirs += [os.path.join('/lib')]
    libraries += ["boost_python-mt" if BOOST_PYTHON_MT else "boost_python", v8_lib, "c"]

elif os.name == "posix" and sys.platform == "darwin": # contribute by progrium and alec
    # force x64 because Snow Leopard's native Python is 64-bit
    # scons arch=x64 library=static

    include_dirs += [
        "/opt/local/include", # use MacPorts to install Boost
    ]

    library_dirs += [
        V8_HOME,
    ]

    libraries += ["boost_python-mt", v8_lib]

    architecture = platform.architecture()[0]

    if architecture == '64bit':
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
        try:
            from pysvn import Client, Revision, opt_revision_kind

            svnClient = Client()

            r = svnClient.checkout(V8_SVN_URL, V8_HOME, revision=Revision(opt_revision_kind.number, V8_SVN_REVISION))

            if r: return

            print "ERROR: Failed to export from V8 svn repository"            
        except ImportError:
            #print "WARN: could not import pysvn. Ensure you have the pysvn package installed."
            #print "      on debian/ubuntu, this is called 'python-svn'; on Fedora/RHEL, this is called 'pysvn'."

            print "INFO: we will try to use the system 'svn' command to checkout/update V8 code"

        if not os.path.isdir(V8_HOME):
            os.makedirs(V8_HOME)

            cmd = "co"
        else:
            cmd = "up"

        args = ["svn", cmd, V8_SVN_URL, V8_HOME]

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

        x64 = [k for k, v in macros if k == 'V8_TARGET_ARCH_X64']
        
        fixed_build_script = build_script.replace('-fno-rtti', '')\
                                         .replace('-fno-exceptions', '')\
                                         .replace('/WX', '')

        if x64 and os.name != 'nt':
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

        mode = 'debug' if DEBUG else 'release'
        arch = 'x64' if x64 else 'ia32'
        snapshot = 'on' if V8_SNAPSHOT_ENABLED else 'off'

        print "INFO: building Google v8 with SCons for %s platform" % arch

        proc = subprocess.Popen("scons arch=%s mode=%s snapshot=%s" % (arch, mode, snapshot),
                                cwd=V8_HOME, shell=True, stdout=sys.stdout, stderr=sys.stderr)

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
