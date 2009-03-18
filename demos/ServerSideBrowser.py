#!/usr/bin/env python
from __future__ import with_statement

import sys, traceback, os, os.path
import xml.dom.minidom
import logging

import urllib2, urlparse

import BeautifulSoup
import PyV8, w3c

class JavaLibrary(PyV8.JSClass):
    class LangLibrary(PyV8.JSClass):
        def Runnable(self, args):
            logging.debug("new java.lang.Runnable(%s)" % args)
            
        def Thread(self, runnable):
            logging.debug("new java.lang.Thread(%s)" % runnable)
            
    lang = LangLibrary()
    
    class UtilLibrary(PyV8.JSClass):
        def HashMap(self):
            logging.debug("new java.util.HashMap()")
            
            class HashMapWrapper(PyV8.JSClass):
                map = {}
                
                def get(self, key):
                    return self.map[key]
                
                def put(self, key, value):
                    self.map[key] = value
                    
                def containsKey(self, key):
                    return self.map.has_key(key)
                    
            return HashMapWrapper()
    
    util = UtilLibrary()
    
    class IoLibrary(PyV8.JSClass):
        def File(self, filename):
            logging.debug("new java.io.File(%s)" % filename)
            
            class FileWrapper(PyV8.JSClass):
                def __init__(self, filename):
                    self.filename = filename
                    
                def toURL(self):
                    pass
            
            return FileWrapper(filename)
    
    io = IoLibrary()
    
    class NetLibrary(PyV8.JSClass):
        def URL(self, location, url):
            logging.debug("new java.net.URL(%s, %s)" % (location, url))
        
    net = NetLibrary()    

class HtmlStyle(PyV8.JSClass):
    def __init__(self, node):
        self._node = node
        self._attrs = self.parse(node.getAttribute("style"))        
        
    def parse(self, style):
        attrs = {}
                
        try:
            for attr in style.split(';'):
                if attr == '': continue
                
                strs = attr.split(':')
                
                if len(strs) == 2:
                    attrs[strs[0]] = strs[1]
                else:
                    attrs[attr] = None
        except:
            logging.warn("fail to parse the style attribute: %s", sys.exc_info()[1])            
        
        return attrs
    
    def __getattr__(self, name):
        try:
            try:            
                return object.__getattribute__(self, name)
            except AttributeError:            
                return object.__getattribute__(self, "_attrs")[name]
        except:
            logging.error(sys.exc_info())                
        
    def __setattr__(self, name, value):
        try:
            if name[0] == '_':
                return object.__setattr__(self, name, value)
            else:
                node = object.__getattribute__(self, "_node")
                attrs = object.__getattribute__(self, "_attrs")
                style = ";".join(["%s:%s" % (k, v) if v else k for k, v in attrs.items()])
                
                if node.hasAttribute("style") and len(style) == 0:
                    node.removeAttribute("style")
                elif len(style) > 0:
                    node.setAttribute("style", style)
        except:
            logging.error(sys.exc_info())
            
class HtmlLocation(PyV8.JSClass):
    def __init__(self, page):
        self.parts = urlparse.urlparse(page.url, "http")
    
    @property
    def href(self):
        return urlparse.urlunparse(self.parts)
        
    @property
    def protocol(self):
        return self.parts.scheme
    
    @property
    def host(self):
        return self.parts.netloc
    
    @property
    def hostname(self):
        return self.parts.hostname
    
    @property
    def port(self):
        return self.parts.port
        
    @property
    def pathname(self):
        return self.parts.path
    
    @property
    def search(self):
        return self.parts.query
        
    @property
    def hash(self):
        return self.parts.fragment
    
    def assign(self, url):
        """Loads a new HTML document."""
        pass
    
    def reload(self, bReloadSource=False):
        """Reloads the current page."""
        pass
    
    def replace(self, url):
        """Replaces the current document by loading another document at the specified URL."""
        pass

class HtmlWindow(PyV8.JSClass):
    timers = []
    
    def __init__(self, page, dom):
        self.page = page
        self.doc = w3c.getDOMImplementation(dom)        
        self.ctxt = PyV8.JSContext(self)
        
    @property
    def window(self):
        return self
    
    @property
    def document(self):
        return self.doc
    
    def getLocation(self):
        return HtmlLocation(self.page)
    
    def setLocation(self, url):
        pass
    
    location = property(getLocation, setLocation)
    
    def setTimeout(self, code, interval, lang="JavaScript"):
        self.timers.append((interval, code))
        
        return len(self.timers)-1
        
    def clearTimeout(self, idx):
        self.timers[idx] = None
        
    def eval(self, code):
        logging.debug("evalute script: %s...", code[:20])

        return PyV8.JSEngine().compile(str(code)).run()
            
    def execute(self, func):
        logging.debug("evalute function: %s...", str(func)[:20])
        
        return func()
        
class Task(object):
    @staticmethod
    def waitAll(tasks):
        pass

class FetchFile(Task):
    def __init__(self, url):
        self.url = url
        
    def __call__(self):
        logging.debug("fetching from %s", self.url)
        
        return urllib2.urlopen(self.url)

class Evaluator(Task):
    def __init__(self, target):
        assert hasattr(target, "eval")
        
        self.target = target
        
    def __call__(self):
        self.target.eval(self.pipeline)
        
        return self.target
    
    def __repr__(self):
        return "<Evaluator object for %s at 0x%08X>" % (self.target, id(self))
        
class WebScript(object):
    def __init__(self, page, value):
        self.page = page
        
        if type(value) in [str, unicode]:
            self.script = value
        elif hasattr(value, "read"):
            self.script = value.read()
        else:
            self.func = value
        
    def eval(self, pipeline):       
        if hasattr(self, "script"):
            result = self.page.window.eval(self.script)
        else:
            result = self.page.window.execute(self.func)
        
class WebCss(object):
    def __init__(self, page, value):
        self.page = page
        
        self.css = value if type(value) in [str, unicode] else value.read()
        
    def eval(self, pipeline):
        logging.info("evalute css: %s...", self.css[:20])
        
class WebPage(object):
    children = []
    
    def __init__(self, parent, response):
        self.parent = parent
        
        self.code = response.code
        self.url = response.geturl()
        self.headers = response.headers

        html = response.read()
        
        self.size = len(html)
        self.dom = BeautifulSoup.BeautifulSoup(html)
        
        self.window = HtmlWindow(self, self.dom)
        
    def __repr__(self):
        return "<WebPage at %s>" % self.url
        
    def eval(self, pipeline):
        with self.window.ctxt:
            tasks = []
            
            for iframe in self.dom.findAll('iframe'):
                tasks.append(pipeline.openPage(self, iframe["src"],
                    lambda page: self.children.append((iframe, page))))
            
            for frame in self.dom.findAll('frame'):
                tasks.append(pipeline.openPage(self, frame["src"],
                    lambda page: self.children.append((frame, page))))
            
            for link in self.dom.findAll('link', rel='stylesheet', type='text/css', href=True):
                tasks.append(pipeline.openCss(self, link["href"],
                    lambda css: self.children.append((link, css))))
                
            for style in self.dom.findAll('style,', type='text/css'):
                tasks.append(pipeline.evalCss(self, unicode(style.string).encode("utf-8"),
                    lambda css: self.children.append((link, css))))
            
            for script in self.dom.findAll('script'):
                if script.has_key("type") and script["type"] != "text/javascript":
                    raise NotImplementedError("not support script type %s", script["type"])
                elif script.has_key("src"):
                    tasks.append(pipeline.openScript(self, script["src"],
                        lambda child: self.children.append((script, child))))
                else:
                    tasks.append(pipeline.evalScript(self, unicode(script.string).encode("utf-8"),
                        lambda child: self.children.append((script, child))))
                    
            Task.waitAll(tasks)
            
            self.window.timers.sort(lambda x, y: x[0] - y[0])
                
            for interval, code in self.window.timers:
                tasks.append(pipeline.evalScript(self, code))
            
            body = self.dom.find('body')
            
            if body and body.has_key('onload'):
                script = body['onload']
                
                if script.startswith("vbscript:"):
                    raise NotImplementedError("not support script type %s", script["type"])
                elif script.startswith("javascript:"):
                    script = script[len("javascript:"):]
                
                tasks.append(pipeline.evalScript(self, script,
                    lambda child: self.children.append((body, child))))

class WebSession(object):
    def __init__(self, root):
        self.root = root
        
    def __repr__(self):
        return "<WebSession at %s>" % self.root.url        
        
class Pipeline(object):
    def __init__(self):
        self.evalPage = self.getEvaluator(WebPage)
        self.openPage = self.getOpener(WebPage)        
        self.evalCss = self.getEvaluator(WebCss)
        self.openCss = self.getOpener(WebCss)
        self.evalScript = self.getEvaluator(WebScript)
        self.openScript = self.getOpener(WebScript)
    
    def queue(self, task, callback):
        try:
            task.pipeline = self            
            task.result = callback(task())
            
            return task
        except:
            logging.error("fail to execute task %s", task)
            logging.debug(traceback.format_exc())
    
    def openSession(self, url, callback):
        self.openPage(None, url,
            lambda page: callback(WebSession(page)))
        
    def getEvaluator(self, clazz):
        def evaluator(parent, target, callback=None):
            self.queue(Evaluator(clazz(parent, target)), 
                lambda result: callback(result) if callback else None)
            
        return evaluator
    
    def getOpener(self, clazz):
        def opener(parent, url, callback=None):
            if parent:
                url = urlparse.urljoin(parent.url, url)
            
            self.queue(FetchFile(url),
                lambda response: self.queue(Evaluator(clazz(parent, response)),
                    lambda result: callback(result) if callback else None))
            
        return opener
    
class Browser(object):
    pipeline = Pipeline()
    sessions = []
    
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
            
    def openUrl(self, url):        
        self.pipeline.openSession(url, lambda session: self.sessions.append(session))
        
    def findSessions(self, pattern):
        for p in pattern.split():       
            try:
                yield self.sessions[int(p)]
            except:
                for s in self.sessions:
                    if s.root.url.find(p) >= 0:
                        yield s
        
    def listSessions(self, pattern):        
        for session in self.findSessions(pattern) if pattern else self.sessions:
            print "#%d\t%s" % (self.sessions.index(session), session.root.url)

    COMMANDS = (
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
        },
        {
            "names" : ["open", "o"],
            "help" : "open a HTML page",
            "handler" : lambda self, line: self.openUrl(line)
        },
        {
            "names" : ["sessions", "s"],
            "help" : "list the web sessions",
            "handler" : lambda self, line: self.listSessions(line)
        }
    )
    
    def runCommand(self, line):
        for command in self.COMMANDS:
            for name in command["names"]:
                if line.startswith(name):
                    if command.has_key("handler"):
                        try:
                            return command["handler"](self, line[len(name):].strip())
                        except:
                            traceback.print_exc()
                            break
                    else:
                        break
                
        for command in self.COMMANDS:
            print "%s    %s" % (", ".join(command["names"]).rjust(15), command["help"])
    
    def runJavascript(self, source):
        try:
            result = PyV8.JSEngine().compile(source).run()
            
            if result:
                print str(result)
        except:
            traceback.print_exc()
            
    def runShellCommand(self, line):
        try:
            os.system(line)
        except:
            traceback.print_exc()

    MODES = {
        "python" : {
            "abbr" : "py"
        },
        "javascript" : {
            "abbr" : "js"
        },
        "shell" : {
            "abbr" : "sh"
        },
    }

    def runShell(self):
        import code
        
        logging.basicConfig(level=self.opts.logLevel,
                            format=self.opts.logFormat)
        
        logging.debug("settings: %s", self.opts)
                
        self.mode = "python"
                
        self.console = code.InteractiveConsole({"sessions" : []})
        
        self.terminated = False
        
        while not self.terminated:
            line = self.console.raw_input(self.MODES[self.mode]["abbr"] + ">").strip()
            
            if len(line) == 0: continue
                            
            if line[0] == '`':                    
                self.runCommand(line[1:])
            elif line[0] == '?':
                self.runJavascript(line[1:])
            elif line[0] == '!':
                self.runShellCommand(line[1:])
            else:
                if self.mode == "python":
                    self.console.runsource(line)
                elif self.mode == "javascript":
                    self.runJavascript(line)
                elif self.mode == "shell":
                    self.runShellCommand(line)
                else:
                    print "unknown mode - " + self.mode

if __name__ == "__main__":
    with Browser() as browser:    
        if browser.parseCmdLine():
            browser.runShell()