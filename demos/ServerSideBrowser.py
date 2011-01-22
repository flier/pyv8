#!/usr/bin/env python
from __future__ import with_statement

import sys, traceback, os, os.path
import xml.dom.minidom
import logging

class Task(object):
    @staticmethod
    def waitAll(tasks):
        pass

class FetchFile(Task):
    def __init__(self, url):
        self.url = url
        
    def __call__(self):
        logging.debug("fetching from %s", self.url)
        
        try:
            return urllib2.urlopen(self.url)
        except:
            logging.warn("fail to fetch %s: %s", self.url, traceback.format_exc())
            
            return None

class Evaluator(Task):
    def __init__(self, target):
        assert hasattr(target, "eval")
        
        self.target = target
        
    def __call__(self):
        try:
            self.target.eval(self.pipeline)
        except:
            logging.warn("fail to evalute %s: %s", self.target, traceback.format_exc())
            
        return self.target
    
    def __repr__(self):
        return "<Evaluator object for %s at 0x%08X>" % (self.target, id(self))
        
class WebObject(object):
    context = []
    
    def __enter__(self):
        self.context.append(self)
        
        logging.debug("entering %s...", self)
        
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.context.pop()
        
        logging.debug("leaving %s...", self)
    
    def __init__(self, parent, url):
        self.children = []
        self.parent = parent
        self.url = url
        
    @staticmethod
    def current():        
        current = WebObject.context[-1] if len(WebObject.context) > 0 else None
                
        return current
        
    @property
    def page(self):        
        tag = self.parent
        
        while not isinstance(tag, WebPage):
            tag = tag.parent
            
        return tag
    
class WebScript(WebObject):
    def __init__(self, parent, value, url):
        WebObject.__init__(self, parent, url)
        
        if type(value) in [str, unicode]:
            self.script = value
        elif hasattr(value, "read"):
            self.script = value.read()
        else:
            self.func = value
        
    def eval(self, pipeline):
        if len(WebObject.context) > 0:
            WebObject.context[-1].children.append((None, self))
        
        with self:
            if hasattr(self, "script"):
                self.result = self.page.window.eval(self.script)
            else:
                self.result = self.page.window.execute(self.func)

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

class WebCss(WebObject):
    def __init__(self, parent, value, url):
        WebObject.__init__(self, parent, url)
        
        self.css = value if type(value) in [str, unicode] else value.read()
        
    def eval(self, pipeline):
        logging.info("evalute css: %s...", self.css[:20])
        
        with self:
            pass
        
class WebPage(WebObject):
    def __init__(self, parent, response, url):
        WebObject.__init__(self, parent, url)
        
        self.code = response.code
        self.headers = response.headers

        html = response.read()
        
        self.size = len(html)
        self.dom = BeautifulSoup.BeautifulSoup(html)
        
        self.window = HtmlWindow(self, self.dom)
        
    def __repr__(self):
        return "<WebPage at %s>" % self.url
    
    def evalScript(self, pipeline, script, parent):        
        if script.has_key("type") and script["type"] != "text/javascript":
            raise NotImplementedError("not support script type %s", script["type"])
        elif script.has_key("src"):
            if script["src"].startswith("http://www.google-analytics.com"):
                return None
            
            return pipeline.openScript(self, script["src"],
                lambda child: parent.children.append((script, child)))
        else:
            return pipeline.evalScript(self, unicode(script.string).encode("utf-8"),
                lambda child: parent.children.append((script, child)))
                    
    def evalTag(self, pipeline, tag, parent):        
        with parent:
            tasks = []            
            
            for iframe in tag.findAll('iframe'):
                tasks.append(pipeline.openPage(self, iframe["src"],
                    lambda page: parent.children.append((iframe, page))))
            
            for frame in tag.findAll('frame'):
                tasks.append(pipeline.openPage(self, frame["src"],
                    lambda page: parent.children.append((frame, page))))
            
            for link in tag.findAll('link', rel='stylesheet', type='text/css', href=True):
                tasks.append(pipeline.openCss(self, link["href"],
                    lambda css: parent.children.append((link, css))))
                
            for style in tag.findAll('style,', type='text/css'):
                tasks.append(pipeline.evalCss(self, unicode(style.string).encode("utf-8"),
                    lambda css: parent.children.append((link, css))))
    
            for script in tag.findAll('script'):
                tasks.append(self.evalScript(pipeline, script, parent))
    
        return tasks
            
    def eval(self, pipeline):
        with self.window.ctxt:
            scripts = []
                
            self.window.document.onCreateElement = lambda element: scripts.append((element, WebObject.current())) if element.tagName == "script" else None
            self.window.document.onDocumentWrite = lambda element: self.evalTag(pipeline, element.tag, WebObject.current())
            
            tasks = self.evalTag(pipeline, self.dom, self)
                    
            Task.waitAll(tasks)
            
            self.window.timers.sort(lambda x, y: x[0] - y[0])
                
            for interval, code in self.window.timers:
                tasks.append(pipeline.evalScript(self, code))
                
            try:
                scripts.append((self.window.document.body['onload'], self))
            except:
                pass
                
            for script, parent in scripts:
                with parent:
                    tasks.append(self.evalScript(pipeline, script.tag, parent))
            
class WebSession(object):
    def __init__(self, root):
        self.root = root
        
    def __repr__(self):
        return "<WebSession at %s>" % self.root.url
    
    def dumpName(self, obj):
        if isinstance(obj, WebCss): return "Css%d" % id(obj)
        if isinstance(obj, WebScript): return "Script%d" % id(obj)
        if isinstance(obj, WebPage): return "Page%d" % id(obj)
        return "Object%d" % id(obj)
    
    def dumpChildren(self, out, obj):
        for tag, child in obj.children:
            if isinstance(child, WebCss):
                self.dumpCss(out, child)
            elif isinstance(child, WebScript):
                self.dumpScript(out, child)
            elif isinstance(child, WebPage):
                self.dumpPAge(out, child)
    
    def dumpCss(self, out, css):
        print >>out, '%s [label="%s"];' % (self.dumpName(css), css.url or "inline CSS")
        print >>out, '%s -> %s;' % (self.dumpName(css.parent), self.dumpName(css))
        
        self.dumpChildren(out, css)
        
    def dumpScript(self, out, script):
        print >>out, '%s [label="%s"];' % (self.dumpName(script), script.url or "inline Script")
        print >>out, '%s -> %s;' % (self.dumpName(script.parent), self.dumpName(script))
        
        self.dumpChildren(out, script)
    
    def dumpPage(self, out, page):
        print >>out, '%s [label="%s"];' % (self.dumpName(page), page.url)
        
        self.dumpChildren(out, page)        
    
    def save(self, filename):
        with open(filename, "w") as f:
            print >>f, "digraph WebSession {"
            
            self.dumpPage(f, self.root)
            
            print >>f, "}"
        
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
            
            result = task()
            
            if result:
                task.result = callback(result)
            
            return task
        except:
            logging.error("fail to execute task %s", task)
            logging.debug(traceback.format_exc())
    
    def openSession(self, url, callback):
        self.openPage(None, url,
            lambda page: callback(WebSession(page)))
        
    def getEvaluator(self, clazz):
        def evaluator(parent, target, callback=None):
            self.queue(Evaluator(clazz(parent, target, None)), 
                lambda result: callback(result) if callback else None)
            
        return evaluator
    
    def getOpener(self, clazz):
        def opener(parent, url, callback=None):
            if parent:
                url = urlparse.urljoin(parent.url, url)
            
            self.queue(FetchFile(url),
                lambda response: self.queue(Evaluator(clazz(parent, response, url)),
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
        },
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
                
        self.console = code.InteractiveConsole({"sessions" : self.sessions})
        
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