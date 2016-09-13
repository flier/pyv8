#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement
from __future__ import print_function

import os.path
import subprocess
import traceback
import logging
import urllib
import zipfile
from datetime import datetime
import multiprocessing

import ez_setup
ez_setup.use_setuptools()

from distutils.command.build import build as _build
from setuptools import setup, Extension
from setuptools.command.develop import develop as _develop

from settings import *

log = logging.getLogger()

def check_env():
    if is_cygwin or is_mingw:
        log.error("Cygwin or MingGW is not official support, please try to use Visual Studio 2010 Express or later.")
        sys.exit(-1)

def exec_cmd(cmdline, *args, **kwargs):
    msg = kwargs.get('msg')
    cwd = kwargs.get('cwd', '.')
    output = kwargs.get('output')

    if msg:
        log.info(msg)

    cmdline = ' '.join([cmdline] + list(args))

    rel_path = os.path.relpath(cwd)

    log.debug("exec %s$ %s", rel_path if rel_path != '.' else '', cmdline)

    proc = subprocess.Popen(cmdline,
                            shell=kwargs.get('shell', True),
                            cwd=cwd,
                            env=kwargs.get('env'),
                            stdout=subprocess.PIPE if output else None,
                            stderr=subprocess.PIPE if output else None)

    stdout, stderr = proc.communicate()

    succeeded = proc.returncode == 0

    if not succeeded:
        log.error("%s failed: code=%d", msg or "Execute command", proc.returncode)

        if output:
            log.debug(stderr)

    return succeeded, stdout, stderr if output else succeeded

download_steps = 0

def download_progress(blocks, block_size, total_size):
    global download_steps

    downloaded_bytes = blocks*block_size
    downloaded_percent = float(downloaded_bytes)/total_size

    if int(downloaded_percent*10) > download_steps:
        download_steps += 1
        log.info("< %%%d %d of %d bytes", downloaded_percent*100, downloaded_bytes, total_size)

def install_depot():
    """
    Install depot_tools

    http://dev.chromium.org/developers/how-tos/install-depot-tools
    """
    if os.path.isfile(os.path.join(DEPOT_HOME, 'gclient')):
        _, stdout, _ = exec_cmd("./gclient --version", cwd=DEPOT_HOME, output=True)

        log.info("found depot tools with %s", stdout.strip())

        if os.path.exists(os.path.join(DEPOT_HOME, '.git')):
            exec_cmd("git pull", cwd=DEPOT_HOME)
    elif is_winnt:
        log.info("Downloading depot tools ...")

        start_time = datetime.now()

        tmpfile, _ = urllib.urlretrieve(DEPOT_DOWNLOAD_URL, reporthook=download_progress)

        download_time = datetime.now() - start_time

        stat = os.stat(tmpfile)

        log.info("Download %d bytes in %d seconds, %.2fKB/s",
                 stat.st_size, download_time.seconds, float(stat.st_size)/1024/download_time.seconds)

        try:
            with zipfile.ZipFile(tmpfile) as z:
                z.extractall(DEPOT_HOME)
        finally:
            os.remove(tmpfile)
    elif os.path.isdir(os.path.join(DEPOT_HOME, '.git')):
        exec_cmd("git pull", os.path.basename(DEPOT_HOME), cwd=os.path.dirname(DEPOT_HOME), msg="Cloning depot tools ...")
    elif not os.path.exists(DEPOT_HOME):
        exec_cmd("git clone", DEPOT_GIT_URL, DEPOT_HOME, cwd=os.path.dirname(DEPOT_HOME), msg="Cloning depot tools ...")
    else:
        log.info("Skip depot_tools folder without .git")


def sync_v8():
    """
    Using Git

    https://github.com/v8/v8/wiki/Using%20Git
    """
    if os.path.isdir(V8_HOME):
        exec_cmd(os.path.join(DEPOT_HOME, 'gclient'), 'sync', cwd=V8_HOME, msg="Sync Google V8 code...")
    else:
        os.makedirs(V8_HOME)

        exec_cmd(os.path.join(DEPOT_HOME, 'fetch'), 'v8', cwd=os.path.dirname(V8_HOME), msg="Fetch Google V8 code...")


def checkout_v8():
    """
    Working with Release Branches

    https://www.chromium.org/developers/how-tos/get-the-code/working-with-release-branches
    """
    if not OFFLINE_MODE:
        exec_cmd('git fetch --tags', cwd=V8_HOME, msg='Fetch the release tag information')

    exec_cmd('git checkout', V8_GIT_TAG, cwd=V8_HOME, msg='Checkout Google V8 v' + V8_GIT_TAG)

def patch_gyp():
    """
    Patch build/common.gypi to enable RTTI and C++ exceptions
    """
    log.info("Patching the GYP scripts")

    # Next up, we have to patch the SConstruct file from the v8 source to remove -no-rtti and -no-exceptions
    gypi = os.path.join(V8_HOME, "build/common.gypi")

    # Check if we need to patch by searching for rtti flag in the data
    with open(gypi, 'r') as f:
        build_script = f.read()

    patched_build_script = build_script.replace("'-fno-rtti'", "") \
                                       .replace("'-fno-exceptions'", "") \
                                       .replace("'-Werror'", "") \
                                       .replace("'RuntimeTypeInfo': 'false',", "'RuntimeTypeInfo': 'true',") \
                                       .replace("'ExceptionHandling': '0',", "'ExceptionHandling': '1',") \
                                       .replace("'GCC_ENABLE_CPP_EXCEPTIONS': 'NO'", "'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'") \
                                       .replace("'GCC_ENABLE_CPP_RTTI': 'NO'", "'GCC_ENABLE_CPP_RTTI': 'YES'")

    if build_script == patched_build_script:
        log.info("skip patched build/common.gypi file ")
    else:
        log.info("patch build/common.gypi file to enable RTTI and C++ exceptions")

        if os.path.exists(gypi + '.bak'):
            os.remove(gypi + '.bak')

        os.rename(gypi, gypi + '.bak')

        with open(gypi, 'w') as f:
            f.write(patched_build_script)


def build_v8():
    """
    Building with Gyp

    https://github.com/v8/v8/wiki/Building%20with%20Gyp
    """
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
        'i18nsupport': 'on' if V8_I18N else 'off',
        'visibility': 'on',
        'library': 'shared',
    }

    if is_winnt:
        log.error("Windows build is not support now, coming soon")
    else:
        target = '%s.%s' % (arch, mode)

        log.info("Building Google v8 with GYP for %s target", target)

        kwargs = ["%s=%s" % (k, v) for k, v in options.items()]

        exec_cmd(MAKE, '-j', str(multiprocessing.cpu_count()), target, *kwargs, cwd=V8_HOME, msg="build v8 from GIT " + V8_GIT_TAG)


def generate_probes():
    build_path = os.path.join(PYV8_HOME, "build")

    if not os.path.exists(build_path):
        log.info("automatic make the build folder: %s", build_path)

        try:
            os.makedirs(build_path, 0755)
        except os.error as ex:
            log.warn("fail to create the build folder, %s", ex)

    probes_d = os.path.join(PYV8_HOME, "src/probes.d")
    probes_h = os.path.join(PYV8_HOME, "src/probes.h")
    probes_o = os.path.join(build_path, "probes.o")

    if is_osx and exec_cmd("dtrace -h -xnolibs -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes"):
        pass
    elif (is_linux or is_freebsd) and \
         (exec_cmd("dtrace -h -C -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes.h") and \
          exec_cmd("dtrace -G -C -s %s -o %s" % (probes_d, probes_o), "generate DTrace probes.o")):
        extra_objects.append(probes_o)
    else:
        log.info("dtrace or systemtap doesn't works, force to disable probes")

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
        check_env()

        if not OFFLINE_MODE:
            install_depot()
            sync_v8()
        
        checkout_v8()

        patch_gyp()
        build_v8()

        #generate_probes()
    except Exception as e:
        log.error("fail to checkout and build v8, %s", e)
        log.debug(traceback.format_exc())


class build(_build):
    def run(self):
        prepare_v8()

        _build.run(self)


class develop(_develop):
    def run(self):
        prepare_v8()

        _develop.run(self)

if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s [%(process)d] %(levelname)s %(message)s',
                        level=logging.DEBUG if PYV8_DEBUG else logging.INFO,
                        stream=sys.stderr)

    source_files = ["Utils.cpp", "Exception.cpp", "Context.cpp", "Engine.cpp", "Wrapper.cpp",
                    "Debug.cpp", "Locker.cpp", "PyV8.cpp"]

    if V8_AST:
        source_files += ["AST.cpp", "PrettyPrinter.cpp"]

    sources = [os.path.join("src", file) for file in source_files]

    pyv8 = Extension(name="_PyV8",
                    sources=sources,
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
        extra.update(dict(use_2to3=True))

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
        version='2016.9.' + V8_GIT_TAG,
        description='Python Wrapper for Google V8 Engine',
        long_description=description,
        classifiers=classifiers,
        platforms="x86",
        author='Flier Lu',
        author_email='flier.lu@gmail.com',
        url='https://github.com/flier/pyv8',
        download_url='https://github.com/flier/pyv8/releases',
        license="Apache Software License",
        keywords=["javascript", "v8"],
        install_requires=['setuptools'],
        py_modules=['PyV8'],
        ext_modules=[pyv8],
        packages=[''],
        package_dir={
            '': v8_library_path,
        },
        package_data={
            '': ['*.bin', '*.dat'],
        },
        test_suite='PyV8',
        cmdclass=dict(compile=compile, build=build, v8build=_build, develop=develop),
        **extra)
