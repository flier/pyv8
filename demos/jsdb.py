#!/usr/bin/env python
import sys
import os
import os.path
import logging
import threading

import PyV8

class cmd(object):
    def __init__(self, *alias):
        self.alias = alias

    def __call__(self, func):
        def wrapped(*args, **kwds):
            return func(*args, **kwds)

        wrapped.alias = list(self.alias)
        wrapped.alias.append(func.__name__)
        wrapped.__name__ = func.__name__
        wrapped.__doc__ = func.__doc__

        return wrapped

class Debugger(PyV8.JSDebug, threading.Thread):
    log = logging.getLogger("dbg")

    def __init__(self, ctxt, *args, **kwargs):
        PyV8.JSDebug.__init__(self)
        threading.Thread.__init__(self, name='dbg')

        self.terminated = False
        self.ctxt = ctxt
        self.daemon = True

        self.args = args
        self.kwargs = kwargs

        self.cmds = []
        self.alias = {}

        self.buildCmds()

    def showCopyright(self):
        print "jsdb with PyV8 v%s base on Google v8 engine v%s" % (PyV8.__version__, PyV8.JSEngine.version)
        print "Apache License 2.0 <http://www.apache.org/licenses/LICENSE-2.0>"

    def __enter__(self):
        self.enabled = True

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        PyV8.debugger.enabled = False

    def onMessage(self, msg):
        self.log.info("Debug message: %s", msg)

        try:
            if msg['type'] == 'event' and msg['event'] == 'break':
                print self.stepNext()
        except:
            import traceback

            traceback.print_exc()

    def onBreak(self, evt):
        self.log.info("Break event: %s", evt)

    def onException(self, evt):
        self.log.info("Exception event: %s", evt)

    def onNewFunction(self, evt):
        self.log.info("New function event: %s", evt)

    def onBeforeCompile(self, evt):
        self.log.info("Before compile event: %s", evt)

    def onAfterCompile(self, evt):
        self.log.info("After compile event: %s", evt)

    def run(self):
        for arg in self.args:
            if os.path.isfile(arg):
                with open(arg, 'r') as f:
                    logging.info("loading script from %s...", arg)

                    ctxt.eval(f.read())
            else:
                ctxt.eval(arg)

    def shell(self):
        while not self.terminated:
            line = raw_input('(jsdb) ').strip()

            if line == '': continue

            args = line.split(' ')
            cmd = args[0]
            args = args[1:]

            func = self.alias.get(cmd)

            if func is None:
                print "Undefined command: '%s'.  Try 'help'." % cmd
            else:
                try:
                    func(*args)
                except Exception, e:
                    print e

    def buildCmds(self):
        for name in dir(self):
            attr = getattr(self, name)

            if callable(attr) and hasattr(attr, 'alias'):
                self.cmds.append(attr)
                
                for name in attr.alias:
                    self.alias[name] = attr
                    
        self.log.info("found %d commands with %d alias", len(self.cmds), len(self.alias))

    @cmd('h', '?')
    def help(self, cmd=None, *args):
        """Print list of commands."""

        if cmd:
            func = self.alias.get(cmd)

            if func is None:
                print "Undefined command: '%s'.  Try 'help'." % cmd
            else:
                print func.__doc__
        else:
            print "List of commands:"
            print

            for func in self.cmds:
                print "%s -- %s" % (func.__name__, func.__doc__)

    @cmd('q')
    def quit(self):
        """Exit jsdb."""
        self.terminated = True

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
        with Debugger(ctxt, *args) as dbg:
            dbg.showCopyright()
            dbg.breakForDebug()

            dbg.start()

            try:
                dbg.shell()
            except KeyboardInterrupt:
                pass