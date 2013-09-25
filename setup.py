#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement
from __future__ import print_function

import sys, os, os.path, math, platform
import subprocess
import traceback

is_py3k = sys.version_info[0] > 2

is_winnt = os.name == "nt"
is_linux = os.name == "posix" and sys.platform.startswith("linux")
is_freebsd = os.name == "posix" and sys.platform.startswith("freebsd")
is_mac = os.name == "mac"
is_osx = os.name == "posix" and sys.platform == "darwin"
is_cygwin = os.name == "posix" and sys.platform == "cygwin"
is_mingw = is_winnt and os.environ.get('MSYSTEM', '').startswith('MINGW')
is_64bit = False
is_arm = False

if is_cygwin or is_mingw:
    print("ERROR: Cygwin or MingGW is not official support, please try to use Visual Studio 2010 Express or later.")
    sys.exit(-1)

if is_py3k:
    import distribute_setup
    distribute_setup.use_setuptools()
else:
    import ez_setup
    ez_setup.use_setuptools()

from distutils.command.build import build as _build
from setuptools import setup, Extension
from setuptools.command.develop import develop as _develop

# default settings, you can modify it in buildconf.py.
# please look in buildconf.py.example for more information
PYV8_HOME = os.path.abspath(os.path.dirname(__file__))
BOOST_HOME = None
BOOST_MT = is_osx
BOOST_STATIC_LINK = False
PYTHON_HOME = None
V8_HOME = None
V8_SVN_URL = "http://v8.googlecode.com/svn/trunk/"
V8_SVN_REVISION = None

v8_svn_rev_file = os.path.normpath(os.path.join(os.path.dirname(__file__), 'REVISION'))

if os.path.exists(v8_svn_rev_file):
    try:
        V8_SVN_REVISION = int(open(v8_svn_rev_file).read().strip())
    except ValueError:
        print("WARN: invalid revision number in REVISION file")

INCLUDE = None
LIB = None
DEBUG = False

MAKE = 'gmake' if is_freebsd else 'make'

V8_SNAPSHOT_ENABLED = not DEBUG # build using snapshots for faster start-up
V8_NATIVE_REGEXP = True         # Whether to use native or interpreted regexp implementation
V8_OBJECT_PRINT = DEBUG         # enable object printing
V8_EXTRA_CHECKS = DEBUG         # enable extra checks
V8_VERIFY_HEAP = DEBUG          # enable verify heap
V8_GDB_JIT = False              # enable GDB jit
V8_VTUNE_JIT = False
V8_DISASSEMBLEER = DEBUG        # enable the disassembler to inspect generated code
V8_DEBUGGER_SUPPORT = True      # enable debugging of JavaScript code
V8_LIVE_OBJECT_LIST = DEBUG     # enable live object list features in the debugger
V8_WERROR = False               # ignore compile warnings
V8_STRICTALIASING = True        # enable strict aliasing
V8_BACKTRACE = True

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
V8_SVN_URL = os.environ.get('V8_SVN_URL', V8_SVN_URL)
INCLUDE = os.environ.get('INCLUDE', INCLUDE)
LIB = os.environ.get('LIB', LIB)
DEBUG = os.environ.get('DEBUG', DEBUG)
MAKE = os.environ.get('MAKE', MAKE)

if type(DEBUG) == str:
    DEBUG = DEBUG.lower() in ['true', 'on', 't']

if V8_HOME is None or not os.path.exists(os.path.join(V8_HOME, 'include', 'v8.h')):
    print("WARN: V8_HOME doesn't exists or point to a wrong folder, ")

    V8_HOME = os.path.join(PYV8_HOME, 'build', ('v8_r%d' % V8_SVN_REVISION) if V8_SVN_REVISION else 'v8')

    svn_name = None
else:
    svn_name = ('SVN r%d' % V8_SVN_REVISION) if V8_SVN_REVISION else 'the latest SVN trunk'

    print("INFO: Found Google v8 base on V8_HOME <%s>" % V8_HOME)

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

if V8_LIVE_OBJECT_LIST:
    V8_OBJECT_PRINT = True
    V8_DEBUGGER_SUPPORT = True
    macros += [("LIVE_OBJECT_LIST", None)]

if V8_OBJECT_PRINT:
    macros += [("OBJECT_PRINT", None)]

if V8_DEBUGGER_SUPPORT:
    macros += [("ENABLE_DEBUGGER_SUPPORT", None)]

boost_libs = ['boost_python', 'boost_thread', 'boost_system']

if BOOST_MT:
    boost_libs = [lib + '-mt' for lib in boost_libs]

include_dirs = [
    os.path.join(V8_HOME, 'include'),
    V8_HOME,
    os.path.join(V8_HOME, 'src'),
]
library_dirs = []
libraries = []
extra_compile_args = []
extra_link_args = []
extra_objects = []

if INCLUDE:
    include_dirs += [p for p in INCLUDE.split(os.path.pathsep) if p]
if LIB:
    library_dirs += [p for p in LIB.split(os.path.pathsep) if p]

if is_winnt:
    import platform
    is_64bit = platform.architecture()[0] == "64bit"

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

    if DEBUG:
        extra_compile_args += ["/Od", "/MTd", "/EHsc", "/Gy", "/Zi"]
    else:
        extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]

    extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X64" if is_64bit else "/MACHINE:X86"]

    if DEBUG:
        extra_link_args += ["/DEBUG"]

    os.putenv('MSSdk', 'true')
    os.putenv('DISTUTILS_USE_SDK', 'true')
elif is_linux or is_freebsd:
    if BOOST_HOME:
        boost_lib_dir = os.path.join(BOOST_HOME, 'stage/lib')
        include_dirs += [BOOST_HOME]
    else:
        boost_lib_dir = '/usr/local/lib'
        include_dirs += ['/usr/local/include']

    library_dirs += [
        boost_lib_dir,
    ]

    if PYTHON_HOME:
        major, minor, _, _, _ = sys.version_info
        library_dirs += [os.path.join(PYTHON_HOME, 'lib/python%d.%d' % (major, minor))]
        include_dirs += [os.path.join(PYTHON_HOME, 'include')]

    extra_compile_args += ["-Wno-write-strings"]

    if BOOST_STATIC_LINK:
        extra_link_args += [os.path.join(boost_lib_dir, "lib%s.a") % lib for lib in boost_libs]
    else:
        libraries += boost_libs

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

    if DEBUG:
        extra_compile_args += ['-g', '-O0', '-fno-inline']
    else:
        extra_compile_args += ['-g', '-O3']

elif is_mac: # contribute by Artur Ventura
    include_dirs += [
        BOOST_HOME,
    ]
    library_dirs += [os.path.join('/lib')]
    libraries += boost_libs + ["c"]

elif is_osx: # contribute by progrium and alec
    # force x64 because Snow Leopard's native Python is 64-bit
    # scons arch=x64 library=static

    if BOOST_HOME:
        include_dirs += [BOOST_HOME]
        library_dirs += [os.path.join(BOOST_HOME, 'stage/lib')]
    else:
        include_dirs += [
            "/opt/local/include", # MacPorts$ sudo port install boost
            "/usr/local/include", # HomeBrew$ brew install boost
        ]

        # MacPorts$ sudo port install boost
        if os.path.isdir("/opt/local/lib"):
            library_dirs.append("/opt/local/lib")

        # HomeBrew$ brew install boost
        if os.path.isdir("/usr/local/lib"):
            library_dirs.append("/usr/local/lib")

    libraries += boost_libs

    is_64bit = math.trunc(math.ceil(math.log(sys.maxsize, 2)) + 1) == 64 # contribute by viy

    if is_64bit:
        os.environ['ARCHFLAGS'] = '-arch x86_64'
        extra_link_args += ["-fPIC"]
        macros += [("V8_TARGET_ARCH_X64", None)]
    else:
        os.environ['ARCHFLAGS'] = '-arch i386'
        macros += [("V8_TARGET_ARCH_IA32", None)]

    if DEBUG:
        extra_compile_args += ['-g', '-O0', '-fno-inline']
    else:
        extra_compile_args += ['-g', '-O3']

else:
    print("ERROR: unsupported OS (%s) and platform (%s)" % (os.name, sys.platform))

arch = 'x64' if is_64bit else 'arm' if is_arm else 'ia32'
mode = 'debug' if DEBUG else 'release'

libraries += ['v8_base.' + arch, 'v8_snapshot' if V8_SNAPSHOT_ENABLED else ('v8_nosnapshot.' + arch)]

if is_winnt:
    library_path = "%s/build/%s/lib/" % (V8_HOME, mode)

elif is_linux or is_freebsd:
    library_path = "%s/out/%s.%s/obj.target/tools/gyp/" % (V8_HOME, arch, mode)
    native_path = "%s/out/native/obj.target/tools/gyp/" % V8_HOME

elif is_osx:
    library_path = "%s/out/%s.%s/" % (V8_HOME, arch, mode)
    native_path = "%s/out/native/" % V8_HOME

library_dirs.append(library_path)

if os.path.isdir(native_path):
    library_dirs.append(native_path)

extra_objects += ["%slib%s.a" % (library_path, name) for name in ['icui18n', 'icuuc', 'icudata']]


def exec_cmd(cmdline_or_args, msg, shell=True, cwd=V8_HOME):
    print("-" * 20)
    print("INFO: %s ..." % msg)
    print("DEBUG: > %s" % cmdline_or_args)

    proc = subprocess.Popen(cmdline_or_args, shell=shell, cwd=cwd, stderr=subprocess.PIPE)

    out, err = proc.communicate()

    succeeded = proc.returncode == 0

    if not succeeded:
        print("ERROR: %s failed: code=%d" % (msg or "Execute command", proc.returncode))
        print("DEBUG: %s" % err)

    return succeeded


def checkout_v8():
    if svn_name:
        print("INFO: we will try to update v8 to %s at <%s>" % (svn_name, V8_SVN_URL))
    else:
        print("INFO: we will try to checkout and build a private v8 build from <%s>." % V8_SVN_URL)

    print("=" * 20)
    print("INFO: Checking out or Updating Google V8 code from SVN...\n")

    update_code = os.path.isdir(V8_HOME) and os.path.exists(os.path.join(V8_HOME, 'include', 'v8.h'))

    try:
        from pysvn import Client, Revision, opt_revision_kind

        svnClient = Client()
        rev = Revision(opt_revision_kind.number, V8_SVN_REVISION) if V8_SVN_REVISION else Revision(opt_revision_kind.head)

        if update_code:
            r = svnClient.update(V8_HOME, revision=rev)
        else:
            r = svnClient.checkout(V8_SVN_URL, V8_HOME, revision=rev)

        if r:
            print("%s Google V8 code (r%d) from SVN to %s" % ("Update" if update_code else "Checkout", r[-1].number, V8_HOME))
            return

        print("ERROR: Failed to export from V8 svn repository")
    except ImportError:
        #print "WARN: could not import pysvn. Ensure you have the pysvn package installed."
        #print "      on debian/ubuntu, this is called 'python-svn'; on Fedora/RHEL, this is called 'pysvn'."

        print("INFO: we will try to use the system 'svn' command to checkout/update V8 code")

    if update_code:
        args = ["svn", "up", V8_HOME]
    else:
        os.makedirs(V8_HOME)

        args = ["svn", "co", V8_SVN_URL, V8_HOME]

    if V8_SVN_REVISION:
        args += ['-r', str(V8_SVN_REVISION)]

    cmdline = ' '.join(args)

    exec_cmd(cmdline, "checkout or update Google V8 code from SVN")


def prepare_gyp():
    print("=" * 20)
    print("INFO: Installing or updating GYP...")

    try:
        if is_winnt:
            if os.path.isdir(os.path.join(V8_HOME, 'build', 'gyp')):
                cmdline = 'svn up build/gyp'
            else:
                cmdline = 'svn co http://gyp.googlecode.com/svn/trunk build/gyp'
        else:
            cmdline = MAKE + ' dependencies'

        exec_cmd(cmdline, "Check out GYP from SVN")
    except Exception as e:
        print("ERROR: fail to install GYP: %s" % e)
        print("       http://code.google.com/p/v8/wiki/BuildingWithGYP")

        sys.exit(-1)


def build_v8():
    print("=" * 20)
    print("INFO: Patching the GYP scripts")

    # Next up, we have to patch the SConstruct file from the v8 source to remove -no-rtti and -no-exceptions
    gypi = os.path.join(V8_HOME, "build/standalone.gypi")

    # Check if we need to patch by searching for rtti flag in the data
    with open(gypi, 'r') as f:
        build_script = f.read()

    fixed_build_script = build_script.replace('-fno-rtti', '') \
                                     .replace('-fno-exceptions', '') \
                                     .replace('-Werror', '') \
                                     .replace("'RuntimeTypeInfo': 'false',", "'RuntimeTypeInfo': 'true',") \
                                     .replace("'ExceptionHandling': '0',", "'ExceptionHandling': '1',") \
                                     .replace("'GCC_ENABLE_CPP_EXCEPTIONS': 'NO'", "'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'") \
                                     .replace("'GCC_ENABLE_CPP_RTTI': 'NO'", "'GCC_ENABLE_CPP_RTTI': 'YES'")

    if build_script == fixed_build_script:
        print("INFO: skip to patch the Google v8 build/standalone.gypi file ")
    else:
        print("INFO: patch the Google v8 build/standalone.gypi file to enable RTTI and C++ Exceptions")

        if os.path.exists(gypi + '.bak'):
            os.remove(gypi + '.bak')

        os.rename(gypi, gypi + '.bak')

        with open(gypi, 'w') as f:
            f.write(fixed_build_script)

    options = {
        'disassembler': 'on' if V8_DISASSEMBLEER else 'off',
        'objectprint': 'on' if V8_OBJECT_PRINT else 'off',
        'verifyheap': 'on' if V8_VERIFY_HEAP else 'off',
        'snapshot': 'on' if V8_SNAPSHOT_ENABLED else 'off',
        'extrachecks': 'on' if V8_EXTRA_CHECKS else 'off',
        'gdbjit': 'on' if V8_GDB_JIT else 'off',
        'vtunejit': 'on' if V8_VTUNE_JIT else 'off',
        'liveobjectlist': 'on' if V8_LIVE_OBJECT_LIST else 'off',
        'debuggersupport': 'on' if V8_DEBUGGER_SUPPORT else 'off',
        'regexp': 'native' if V8_NATIVE_REGEXP else 'interpreted',
        'strictaliasing': 'on' if V8_STRICTALIASING else 'off',
        'werror': 'yes' if V8_WERROR else 'no',
        'backtrace': 'on' if V8_BACKTRACE else 'off',
        'visibility': 'on',
        'component': 'static_library',
    }

    print("=" * 20)

    if is_winnt:
        options['env'] = '"PATH:%PATH%,INCLUDE:%INCLUDE%,LIB:%LIB%"'

        if not os.path.isdir(os.path.join(V8_HOME, 'third_party', 'cygwin')):
            cmdline = 'svn co http://src.chromium.org/svn/trunk/deps/third_party/cygwin@66844 third_party/cygwin'
        else:
            cmdline = 'svn up third_party/cygwin'

        exec_cmd(cmdline, "Update Cygwin from SVN")

        if not os.path.isdir(os.path.join(V8_HOME, 'third_party', 'python_26')):
            cmdline = 'svn co http://src.chromium.org/svn/trunk/tools/third_party/python_26@89111 third_party/python_26'
        else:
            cmdline = 'svn up third_party/python_26'

        exec_cmd(cmdline, "Update Python 2.6 from SVN")

        print("INFO: Generating the Visual Studio project files")

        exec_cmd('third_party\python_26\python.exe build\gyp_v8 -Dtarget_arch=%s' % arch, "Generate Visual Studio project files")

        VSINSTALLDIR = os.getenv('VSINSTALLDIR')

        if VSINSTALLDIR:
            cmdline = '"%sCommon7\\IDE\\devenv.com" /build "%s|%s" build\All.sln' % (VSINSTALLDIR, mode.capitalize(), 'x64' if is_64bit else 'Win32')
        else:
            cmdline = 'devenv.com" /build %s build\All.sln' % mode

        exec_cmd(cmdline, "build v8 from SVN")
    else:
        print("INFO: building Google v8 with GYP for %s platform with %s mode" % (arch, mode))

        options = ' '.join(["%s=%s" % (k, v) for k, v in options.items()])

        cmdline = "%s -j 8 %s %s.%s" % (MAKE, options, arch, mode)

        exec_cmd(cmdline, "build v8 from SVN")


def generate_probes():
    probes_d = os.path.join(PYV8_HOME, "src/probes.d")
    probes_h = os.path.join(PYV8_HOME, "src/probes.h")
    probes_o = os.path.join(PYV8_HOME, "build/probes.o")

    if is_osx and exec_cmd("dtrace -h -xnolibs -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes"):
        pass
    elif (is_linux or is_freebsd) and \
         (exec_cmd("dtrace -h -C -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes.h") and \
          exec_cmd("dtrace -G -C -s %s -o %s" % (probes_d, probes_o), "generate DTrace probes.o")):
        extra_objects.append(probes_o)
    else:
        print("INFO: dtrace or systemtap doesn't works, force to disable probes")

        config_file = os.path.join(PYV8_HOME, "src/Config.h")

        with open(config_file, "r") as f:
            config_settings= f.read()

        modified_config_settings = config_settings.replace("\n#define SUPPORT_PROBES 1", "\n//#define SUPPORT_PROBES 1")

        if modified_config_settings != config_settings:
            if os.path.exists(config_file + '.bak'):
                os.remove(config_file + '.bak')

            os.rename(config_file, config_file + '.bak')

            with open(config_file, 'w') as f:
                f.write(modified_config_settings)


def prepare_v8():
    try:
        checkout_v8()
        prepare_gyp()
        build_v8()
        generate_probes()
    except Exception as e:
        print("ERROR: fail to checkout and build v8, %s" % e)
        traceback.print_exc()


class build(_build):
    def run(self):
        prepare_v8()

        _build.run(self)


class develop(_develop):
    def run(self):
        prepare_v8()

        _develop.run(self)

pyv8 = Extension(name="_PyV8",
                 sources=[os.path.join("src", file) for file in source_files],
                 define_macros=macros,
                 include_dirs=include_dirs,
                 library_dirs=library_dirs,
                 libraries=libraries,
                 extra_compile_args=extra_compile_args,
                 extra_link_args=extra_link_args,
                 extra_objects=extra_objects,
                 )

extra = {}

if is_py3k:
    extra.update({
        'use_2to3': True
    })

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
    'Programming Language :: Python :: 3',
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

setup(name='PyV8',
      cmdclass = { 'build': build, 'v8build': _build, 'develop': develop },
      version='1.0-dev',
      description='Python Wrapper for Google V8 Engine',
      long_description=description,
      platforms="x86",
      author='Flier Lu',
      author_email='flier.lu@gmail.com',
      url='svn+http://pyv8.googlecode.com/svn/trunk/#egg=pyv8-1.0-dev',
      download_url='http://code.google.com/p/pyv8/downloads/list',
      license="Apache Software License",
      install_requires=['setuptools'],
      py_modules=['PyV8'],
      ext_modules=[pyv8],
      test_suite='PyV8',
      classifiers=classifiers,
      **extra)
