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
import json
from itertools import chain
from functools import partial
from datetime import datetime

from distutils.command.build import build
from distutils.command.build_ext import build_ext
from setuptools import setup, Extension
from setuptools.command.develop import develop

from settings import *

log = logging.getLogger()


def check_env():
    if is_cygwin or is_mingw:
        log.error("Cygwin or MingGW is not official support, " +
                  "please try to use Visual Studio 2010 Express or later.")
        sys.exit(-1)


def exec_cmd(cmdline, *args, **kwargs):
    msg = kwargs.get('msg')
    cwd = kwargs.get('cwd', '.')
    output = kwargs.get('output')

    if msg:
        log.info(msg)

    cmdline = ' '.join([cmdline] + list(args))

    rel_path = os.path.relpath(cwd)

    log.debug("exec @ `%s` $ %s", rel_path if rel_path != '.' else '', cmdline)

    proc = subprocess.Popen(cmdline,
                            shell=kwargs.get('shell', True),
                            cwd=cwd,
                            env=kwargs.get('env'),
                            stdout=subprocess.PIPE if output else None,
                            stderr=subprocess.PIPE if output else None)

    stdout, stderr = proc.communicate()

    succeeded = proc.returncode == 0

    if not succeeded:
        log.error("%s failed: code=%d",
                  msg or "Execute command", proc.returncode)

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
        log.info("< %%%d %d of %d bytes",
                 downloaded_percent*100, downloaded_bytes, total_size)


def install_depot():
    """
    Install depot_tools

    http://dev.chromium.org/developers/how-tos/install-depot-tools
    """
    if os.path.isfile(os.path.join(DEPOT_HOME, 'gclient')):
        _, stdout, _ = exec_cmd("./gclient --version",
                                cwd=DEPOT_HOME,
                                output=True)

        log.info("found depot tools with %s", stdout.strip())
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
        log.info("Updating depot tools ...")

        exec_cmd("git pull", os.path.basename(DEPOT_HOME),
                 cwd=os.path.dirname(DEPOT_HOME),
                 msg="Cloning depot tools ...")
    elif not os.path.exists(DEPOT_HOME):
        log.info("Cloning depot tools ...")

        exec_cmd("git clone", DEPOT_GIT_URL, DEPOT_HOME,
                 cwd=os.path.dirname(DEPOT_HOME),
                 msg="Cloning depot tools ...")
    else:
        log.info("Use depot tools folder without .git")


def sync_v8():
    """
    Using Git

    https://github.com/v8/v8/wiki/Using%20Git
    """
    if os.path.isdir(V8_HOME):
        exec_cmd(os.path.join(DEPOT_HOME, 'gclient'), 'sync',
                 cwd=V8_HOME,
                 msg="Sync Google V8 code...")
    else:
        os.makedirs(V8_HOME)

        exec_cmd(os.path.join(DEPOT_HOME, 'fetch'), 'v8',
                 cwd=os.path.dirname(V8_HOME),
                 msg="Fetch Google V8 code...")


def checkout_v8():
    """
    Working with Release Branches

    https://www.chromium.org/developers/how-tos/get-the-code/working-with-release-branches
    """
    if not OFFLINE_MODE:
        exec_cmd('git fetch --tags',
                 cwd=V8_HOME,
                 msg='Fetch the release tag information')

    exec_cmd('git checkout', V8_GIT_TAG,
             cwd=V8_HOME,
             msg='Checkout Google V8 v' + V8_GIT_TAG)


def build_v8():
    """
    Building with Gyp

    https://github.com/v8/v8/wiki/Building%20with%20Gyp
    """
    options = {
        'enable_profiling': PYV8_DEBUG,
        'is_debug': PYV8_DEBUG,
        'symbol_level': 2 if V8_DEBUG_SYMBOLS else 1,
        'v8_enable_backtrace': V8_BACKTRACE,
        'v8_enable_disassembler': V8_DISASSEMBLER,
        'v8_enable_gdbjit': V8_GDB_JIT,
        'v8_enable_handle_zapping': V8_HANDLE_ZAPPING,
        'v8_enable_i18n_support': V8_I18N,
        'v8_enable_inspector': V8_INSPECTOR,
        'v8_enable_object_print': V8_OBJECT_PRINT,
        'v8_enable_slow_dchecks': V8_SLOW_DCHECKS,
        'v8_enable_trace_maps': V8_TRACE_MAPS,
        'v8_enable_v8_checks': V8_EXTRA_CHECKS,
        'v8_enable_verify_csa': V8_VERIFY_CSA,
        'v8_enable_verify_heap': V8_VERIFY_HEAP,
        'v8_enable_verify_predictable': V8_VERIFY_PREDICTABLE,
        'v8_interpreted_regexp': not V8_NATIVE_REGEXP,
        'v8_no_inline': V8_NO_INLINE,
        'v8_use_snapshot': V8_USE_SNAPSHOT,
    }

    if is_winnt:
        log.error("Windows build is not support now, coming soon")
    else:
        log.info("Building Google v8 with GN for %s target", target)

        encoder = json.JSONEncoder()

        args = ['%s=%s' % (k, encoder.encode(v)) for k, v in options.items()]

        exec_cmd(os.path.join(DEPOT_HOME, 'gn'), 'gen', 'out.gn/' + target,
                 "--args='%s'" % ' '.join(args),
                 cwd=V8_HOME, msg="generate build scripts for V8 (v%s)" % V8_GIT_TAG)

        exec_cmd("ninja -C out.gn/%s v8" % target,
                 cwd=V8_HOME, msg="build V8 with ninja")


def generate_probes():
    build_path = os.path.join(PYV8_HOME, "build")

    if not os.path.exists(build_path):
        log.info("automatic make the build folder: %s", build_path)

        try:
            os.makedirs(build_path, 0o755)
        except os.error as ex:
            log.warn("fail to create the build folder, %s", ex)

    probes_d = os.path.join(PYV8_HOME, "src/probes.d")
    probes_h = os.path.join(PYV8_HOME, "src/probes.h")
    probes_o = os.path.join(build_path, "probes.o")

    if is_osx and exec_cmd("dtrace -h -xnolibs -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes"):
        pass
    elif (is_linux or is_freebsd) and \
         (exec_cmd("dtrace -h -C -s %s -o %s" % (probes_d, probes_h), "generate DTrace probes.h") and
          exec_cmd("dtrace -G -C -s %s -o %s" % (probes_d, probes_o), "generate DTrace probes.o")):
        extra_objects.append(probes_o)
    else:
        log.info("dtrace or systemtap doesn't works, force to disable probes")

        config_file = os.path.join(PYV8_HOME, "src/Config.h")

        with open(config_file, "r") as f:
            config_settings = f.read()

        modified_config_settings = config_settings.replace("\n#define SUPPORT_PROBES 1", "\n//#define SUPPORT_PROBES 1")

        if modified_config_settings != config_settings:
            if os.path.exists(config_file + '.bak'):
                os.remove(config_file + '.bak')

            os.rename(config_file, config_file + '.bak')

            with open(config_file, 'w') as f:
                f.write(modified_config_settings)


def prepare_v8():
    try:
        log.info("preparing V8 ...")

        check_env()

        if not OFFLINE_MODE:
            install_depot()

            sync_v8()

        checkout_v8()

        build_v8()

        # generate_probes()
    except Exception as e:
        log.error("fail to checkout and build v8, %s", e)
        log.debug(traceback.format_exc())


class pyv8_build_ext(build_ext):
    def run(self):
        log.info("run `build_ext` command")

        build_ext.run(self)

        self.install()

    def install(self):
        if is_osx:
            for ext in self.extensions:
                log.info("Install `%s` extension", ext.name)

                libs = ['lib%s.dylib' % lib for lib in v8_libs]
                args = chain.from_iterable([['-change', '@rpath/' + lib, '@loader_path/v8/' + lib] for lib in libs])
                dirname, filename = os.path.split(os.path.abspath(self.get_ext_fullpath(ext.name)))
                exec_cmd('install_name_tool', filename, *args,
                         cwd=dirname,
                         msg="Install Extension " + filename)

                for lib_name in libs:
                    args = chain.from_iterable([['-change', '@rpath/' + lib, '@loader_path/' + lib] for lib in libs])

                    exec_cmd('install_name_tool', lib_name, *args,
                             cwd=os.path.join(dirname, 'v8'),
                             msg="Install V8 Library " + lib_name)

class pyv8_build(build):
    def run(self):
        log.info("run `build` command")

        prepare_v8()

        build.run(self)


class pyv8_develop(develop):
    def run(self):
        log.info("run `develop` command")

        prepare_v8()

        develop.run(self)

if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s [%(process)d] %(levelname)s %(message)s',
                        level=logging.DEBUG if PYV8_DEBUG else logging.INFO,
                        stream=sys.stderr)

    source_files = ["Utils.cpp", "Exception.cpp", "Context.cpp", "Engine.cpp",
                    "Wrapper.cpp", "Debug.cpp", "Locker.cpp", "PyV8.cpp"]

    if V8_AST:
        source_files += ["AST.cpp", "PrettyPrinter.cpp"]

    pyv8 = Extension(name="_PyV8",
                     sources=map(partial(os.path.join, "src"), source_files),
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
          version='2.0.0.dev0+' + V8_GIT_TAG,
          description='Python Wrapper for Google V8 Engine',
          long_description=description,
          url='https://github.com/flier/pyv8',
          author='Flier Lu',
          author_email='flier.lu@gmail.com',
          license="Apache Software License",
          classifiers=classifiers,
          keywords=["javascript", "v8"],
          download_url='https://github.com/flier/pyv8/releases',
          install_requires=['setuptools'],
          py_modules=['PyV8'],
          ext_modules=[pyv8],
          packages=['v8'],
          package_dir={
              'v8': v8_library_path,
          },
          package_data={
              'v8': ['*.bin', '*.dat', '*.so', '*.dylib'],
          },
          test_suite='PyV8',
          cmdclass=dict(compile=compile,
                        build_ext=pyv8_build_ext,
                        build=pyv8_build,
                        develop=pyv8_develop),
          **extra)
