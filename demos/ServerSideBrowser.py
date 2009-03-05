#!/usr/bin/env python
from __future__ import with_statement

import sys, os, os.path
import logging

import PyV8

class JavaLibrary(PyV8.JSClass):
    class LangLibrary(PyV8.JSClass):
        def Runnable(self, args):
            logging.info("new java.lang.Runnable(%s)" % args)
            
        def Thread(self, runnable):
            logging.info("new java.lang.Thread(%s)" % runnable)
            
    lang = LangLibrary()
    
    class IoLibrary(PyV8.JSClass):
        def File(self, filename):
            logging.info("new java.io.File(%s)" % filename)
            
            class FileWrapper(PyV8.JSClass):
                def __init__(self, filename):
                    self.filename = filename
                    
                def toURL(self):
                    pass
            
            return FileWrapper(filename)
    
    io = IoLibrary()
    
    class NetLibrary(PyV8.JSClass):
        def URL(self, location, url):
            logging.info("new java.net.URL(%s, %s)" % (location, url))
        
    net = NetLibrary()

class HtmlWindow(PyV8.JSClass):
    pass

class Browser(object):
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        pass
        
    @property
    def version(self):
        return "0.1 (Google v8 engine v%s)" % PyV8.JSEngine.version
        
    def parseCmdLine(self):
        from optparse import OptionParser
        
        parser = OptionParser(version="%prog v" + self.version)
        
        parser.add_option("-q", "--quiet", action="store_const",
                          const=logging.FATAL, dest="logLevel", default=logging.WARN)
        parser.add_option("-v", "--verbose", action="store_const",
                          const=logging.INFO, dest="logLevel")
        parser.add_option("-d", "--debug", action="store_const",
                          const=logging.DEBUG, dest="logLevel")
        parser.add_option("--log-format", dest="logFormat",
                          default="%(asctime)s %(levelname)s %(message)s")

        (self.opts, self.args) = parser.parse_args()
        
        return True
    
    def switchMode(self, mode):
        self.mode = mode
        
    def terminate(self):
        self.terminated = True
        
    def loadJSFile(self, filename):
        logging.info("load javascript file %s" % filename)
        
        with open(filename) as f:
            PyV8.JSEngine().compile(f.read()).run()
        
    def runCommand(self, line):
        commands = (
            {
                "names" : ["javascript", "js"],                
                "help" : "switch to the javascript mode",
                "handler" : lambda self, line: self.switchMode("javascript"),
            },
            {
                "names" : ["python", "py"],                
                "help" : "switch to the python mode",
                "handler" : lambda self, line: self.switchMode("python"),
            },
            {
                "names" : ["shell", "sh"],
                "help" : "switch to the shell mode",
                "handler" : lambda self, line: self.switchMode("shell"),
            },
            {
                "names" : ["exit", "quit", "q"],                
                "help" : "exit the shell",
                "handler" : lambda self, line: self.terminate(),
            },
            {
                "names" : ["help", "?"],
                "help" : "show the help screen"
            },
            
            {
                "names" : ["load", "l"],                    
                "help" : "load javascript file",
                "handler" : lambda self, line: self.loadJSFile(line),
            }
        )
        
        for command in commands:
            for name in command["names"]:
                if line.startswith(name):
                    if command.has_key("handler"):
                        try:
                            return command["handler"](self, line[len(name):].strip())
                        except:
                            print sys.exc_info()
                            break
                    else:
                        break
                
        for command in commands:
            print "%s    %s" % (", ".join(command["names"]).rjust(15), command["help"])
    
    def runJavascript(self, context, source):
        try:
            result = context.eval(source)
            
            if result:
                print str(result)
        except:
            print sys.exc_info()
            
    def runShellCommand(self, line):
        try:
            args = line.split()
            
            os.execv(args[0], args[1:])
        except:
            print sys.exc_info()

    def runShell(self):
        import code
        
        logging.basicConfig(level=self.opts.logLevel,
                            format=self.opts.logFormat)
        
        logging.debug("settings: %s", self.opts)
        
        window = HtmlWindow()
        window.java = JavaLibrary()
        
        self.mode = "python"
                
        with PyV8.JSContext(window) as ctxt:
            console = code.InteractiveConsole({"window" : window})
            
            self.terminated = False
            
            while not self.terminated:           
                if self.mode == "python":
                    line = console.raw_input("py>")
                    
                    if line[0] == '~':
                        self.runCommand(line[1:])
                    elif line[0] == '?':
                        self.runJavascript(ctxt, line[1:])
                    elif line[0] == '!':
                        self.runShellCommand(line[1:])
                    else:
                        console.runsource(line)
                elif self.mode == "javascript":
                    line = console.raw_input("js>")
                    
                    if line[0] == '~':
                        self.runCommand(line[1:])
                    elif line[0] == '?':
                        console.runsource(line[1:])
                    elif line[0] == '!':
                        self.runShellCommand(line[1:])
                    else:
                        self.runJavascript(ctxt, line)
                elif self.mode == "shell":
                    line = console.raw_input("sh>")
                    
                    if line[0] == '~':
                        self.runCommand(line[1:])
                    elif line[0] == '?':
                        console.runsource(line[1:])
                    else:
                        self.runShellCommand(line[1:])

if __name__ == "__main__":
    with Browser() as browser:    
        if browser.parseCmdLine():
            browser.runShell()