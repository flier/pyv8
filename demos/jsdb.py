#!/usr/bin/env python
import sys
import os
import os.path
import logging

import PyV8

class Debugger(PyV8.JSDebug):
    logger = logging.getLogger("dbg")

    def __init__(self, ctxt):
        PyV8.JSDebug.__init__(self)

        self.ctxt = ctxt

    def __enter__(self):
        self.enabled = True

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        PyV8.debugger.enabled = False

    def onMessage(self, msg):
        self.logger.info("Debug message: %s", msg)

        try:
            if msg['type'] == 'event' and msg['event'] == 'break':
                print self.stepNext()
        except:
            import traceback

            traceback.print_exc()

    def onBreak(self, evt):
        self.logger.info("Break event: %s", evt)

    def onException(self, evt):
        self.logger.info("Exception event: %s", evt)

    def onNewFunction(self, evt):
        self.logger.info("New function event: %s", evt)

    def onBeforeCompile(self, evt):
        self.logger.info("Before compile event: %s", evt)

    def onAfterCompile(self, evt):
        self.logger.info("After compile event: %s", evt)

class Shell(PyV8.JSClass):
    def alert(self, txt):
        print txt

def parse_cmdline():
    from optparse import OptionParser

    parser = OptionParser(usage="Usage: %prog [options] <script file>")

    parser.add_option("-q", "--quiet", action="store_const",
                      const=logging.FATAL, dest="logLevel", default=logging.WARN)
    parser.add_option("-v", "--verbose", action="store_const",
                      const=logging.INFO, dest="logLevel")
    parser.add_option("-d", "--debug", action="store_const",
                      const=logging.DEBUG, dest="logLevel")
    parser.add_option("--log-format", dest="logFormat",
                      default="%(asctime)s %(levelname)s %(name)s %(message)s")
    parser.add_option("--log-file", dest="logFile")

    opts, args = parser.parse_args()

    logging.basicConfig(level=opts.logLevel,
                        format=opts.logFormat,
                        filename=opts.logFile)

    return opts, args

if __name__=='__main__':
    opts, args = parse_cmdline()

    with PyV8.JSContext(Shell()) as ctxt:
        with Debugger(ctxt) as dbg:
            dbg.breakForDebug()

            for arg in args:
                if os.path.isfile(arg):
                    with open(arg, 'r') as f:
                        logging.info("loading script from %s...", arg)

                        ctxt.eval(f.read())
                else:
                    ctxt.eval(arg)
