#!/usr/bin/env python
import sys

import logging

import PyV8

__author__ = "flier.lu@gmail.com"
__version__ = "%%prog 0.1 (Google v8 engine v%s)" % PyV8.JSEngine.version

class CommonJsEnv(PyV8.JSClass):
    def __init__(self, parent=None):
        PyV8.JSClass.__init__(self)

        self.parent = parent
        self.exports = {}

    def require(self, name):
        logging.info("loading module <%s>...", name)

        env = CommonJsEnv(self)

        with open(name + '.js', 'r') as f:
            with PyV8.JSContext(env) as ctxt:
                ctxt.eval(f.read())

                return ctxt.locals.exports

    @staticmethod
    def execute(script):
        logging.info("executing script...")

        env = CommonJsEnv()

        with PyV8.JSContext(env) as ctxt:
            return ctxt.eval(script)

def parseCmdline():
    from optparse import OptionParser

    parser = OptionParser(usage="Usage: %prog [options] <scripts>", version=__version__)

    parser.add_option("-q", "--quiet", action="store_const",
                      const=logging.FATAL, dest="logLevel", default=logging.WARN)
    parser.add_option("-v", "--verbose", action="store_const",
                      const=logging.INFO, dest="logLevel")
    parser.add_option("-d", "--debug", action="store_const",
                      const=logging.DEBUG, dest="logLevel")
    parser.add_option("--log-format", dest="logFormat",
                      default="%(asctime)s %(levelname)s %(name)s %(message)s")

    opts, args = parser.parse_args()

    logging.basicConfig(level=opts.logLevel,
                        format=opts.logFormat)

    if len(args) == 0:
        parser.error("missing script files")

    return opts, args

if __name__=='__main__':
    opts, args = parseCmdline()

    for filename in args:
        with open(filename) as f:
            print CommonJsEnv.execute(f.read())