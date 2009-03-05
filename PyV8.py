#!/usr/bin/env python
from __future__ import with_statement

import sys
import StringIO

import _PyV8

__all__ = ["JSClass", "JSEngine", "JSContext", "debugger"]

class JSClass(object):    
    def toString(self):
        "Returns a string representation of an object."
        return "[object %s]" % self.__class__.__name__
    
    def toLocaleString(self):
        "Returns a value as a string value appropriate to the host environment's current locale."
        return self.toString()
    
    def valueOf(self):
        "Returns the primitive value of the specified object."
        return self
    
    def hasOwnProperty(self, name):
        "Returns a Boolean value indicating whether an object has a property with the specified name."
        return hasattr(self, name)
    
    def isPrototypeOf(self, obj):
        "Returns a Boolean value indicating whether an object exists in the prototype chain of another object."
        raise NotImplementedError()
    
    def __defineGetter__(self, name, getter):
        "Binds an object's property to a function to be called when that property is looked up."
        if hasattr(self, name):
            setter = getattr(self, name).fset
        else:
            setter = None
        
        setattr(self, name, property(fget=getter, fset=setter))
    
    def __lookupGetter__(self, name):
        "Return the function bound as a getter to the specified property."
        return self.name.fget
    
    def __defineSetter__(self, name, setter):
        "Binds an object's property to a function to be called when an attempt is made to set that property."
        if hasattr(self, name):
            getter = getattr(self, name).fget
        else:
            getter = None
        
        setattr(self, name, property(fget=getter, fset=setter))
    
    def __lookupSetter__(self, name):
        "Return the function bound as a setter to the specified property."
        return self.name.fset

class JSDebug(object):
    class FrameData(object):
        def __init__(self, frame, count, name, value):
            self.frame = frame
            self.count = count
            self.name = name
            self.value = value
            
        def __len__(self):
            return self.count(self.frame)
        
        def __iter__(self):
            for i in xrange(self.count(self.frame)):
                yield (self.name(self.frame, i), self.value(self.frame, i))
        
    class Frame(object):
        def __init__(self, frame):
            self.frame = frame
            
        @property
        def index(self):
            return int(self.frame.index())
            
        @property
        def function(self):
            return self.frame.func()
            
        @property
        def receiver(self):
            return self.frame.receiver()
            
        @property
        def isConstructCall(self):
            return bool(self.frame.isConstructCall())
            
        @property
        def isDebuggerFrame(self):
            return bool(self.frame.isDebuggerFrame())
            
        @property
        def argumentCount(self):
            return int(self.frame.argumentCount())
                    
        def argumentName(self, idx):
            return str(self.frame.argumentName(idx))
                    
        def argumentValue(self, idx):
            return self.frame.argumentValue(idx)
            
        @property
        def arguments(self):
            return FrameData(self, self.argumentCount, self.argumentName, self.argumentValue)
            
        @property
        def localCount(self, idx):
            return int(self.frame.localCount())
                    
        def localName(self, idx):
            return str(self.frame.localName(idx))
                    
        def localValue(self, idx):
            return self.frame.localValue(idx)
            
        @property
        def locals(self):
            return FrameData(self, self.localCount, self.localName, self.localValue)
            
        @property
        def sourcePosition(self):
            return self.frame.sourcePosition()
            
        @property
        def sourceLine(self):
            return int(self.frame.sourceLine())
            
        @property
        def sourceColumn(self):
            return int(self.frame.sourceColumn())
            
        @property
        def sourceLineText(self):
            return str(self.frame.sourceLineText())
                    
        def evaluate(self, source, disable_break = True):
            return self.frame.evaluate(source, disable_break)
            
        @property
        def invocationText(self):
            return str(self.frame.invocationText())
            
        @property
        def sourceAndPositionText(self):
            return str(self.frame.sourceAndPositionText())
            
        @property
        def localsText(self):
            return str(self.frame.localsText())
            
        def __str__(self):
            return str(self.frame.toText())

    class Frames(object):
        def __init__(self, state):
            self.state = state
            
        def __len__(self):
            return self.state.frameCount
            
        def __iter__(self):
            for i in xrange(self.state.frameCount):                
                yield self.state.frame(i)
    
    class State(object):
        def __init__(self, state):            
            self.state = state
            
        @property
        def frameCount(self):
            return int(self.state.frameCount())
            
        def frame(self, idx = None):
            return JSDebug.Frame(self.state.frame([idx]))
        
        @property    
        def selectedFrame(self):
            return int(self.state.selectedFrame())
        
        @property
        def frames(self):
            return JSDebug.Frames(self)
        
        def __str__(self):
            s = StringIO.StringIO()
            
            try:
                for frame in self.frames:
                    s.write(str(frame))
                    
                return s.getvalue()
            finally:
                s.close()
                
    class DebugEvent(object):
        pass
            
    class StateEvent(DebugEvent):
        __state = None

        @property
        def state(self):
            if not self.__state:
                self.__state = JSDebug.State(self.event.executionState())
            
            return self.__state
        
    class BreakEvent(StateEvent):
        type = _PyV8.JSDebugEvent.Break
        
        def __init__(self, event):
            self.event = event
        
    class ExceptionEvent(StateEvent):
        type = _PyV8.JSDebugEvent.Exception
        
        def __init__(self, event):
            self.event = event

    class NewFunctionEvent(DebugEvent):
        type = _PyV8.JSDebugEvent.NewFunction
        
        def __init__(self, event):
            self.event = event
            
    class Script(object):
        Native = 0
        Extension = 1
        Normal = 2
        
        def __init__(self, script):            
            self.script = script
            
        @property
        def source(self):
            return str(self.script.source())
            
        @property
        def name(self):
            if hasattr(self.script, "name"):
                return str(self.script.name())
            else:
                return None
            
        @property
        def lineOffset(self):
            if hasattr(self.script, "line_offset"):
                return int(self.script.line_offset().value())
            else:
                return -1;
        
        @property    
        def columnOffset(self):
            if hasattr(self.script, "column_offset"):
                return int(self.script.column_offset().value())
            else:
                return -1;
            
        @property
        def type(self):
            return int(self.script.type())
            
        @property
        def typename(self):
            NAMES = {
                JSDebug.Script.Native : "native",
                JSDebug.Script.Extension : "extension",
                JSDebug.Script.Normal : "normal"
            }

            return NAMES[self.type]
            
        def __str__(self):
            return "<%s script %s @ %d:%d> : '%s'" % (self.typename, self.name,
                                                    self.lineOffset, self.columnOffset,
                                                    self.source)
            
    class CompileEvent(StateEvent):
        __script = None

        @property
        def script(self):
            if not self.__script:
                self.__script = JSDebug.Script(self.event.script())
            
            return self.__script
        
        def __str__(self):
            return str(self.script)
            
    class BeforeCompileEvent(CompileEvent):
        type = _PyV8.JSDebugEvent.BeforeCompile
        
        def __init__(self, event):
            self.event = event
        
        def __repr__(self):
            return "before compile script: %s\n%s" % (str(self.script), str(self.state))
    
    class AfterCompileEvent(CompileEvent):
        type = _PyV8.JSDebugEvent.AfterCompile
        
        def __init__(self, event):
            self.event = event

        def __repr__(self):
            return "after compile script: %s\n%s" % (str(self.script), str(self.state))

    onMessage = None
    onBreak = None
    onException = None
    onNewFunction = None
    onBeforeCompile = None
    onAfterCompile = None    
    
    def isEnabled(self):
        return _PyV8.debug().enabled
    
    def setEnabled(self, enable):    
        dbg = _PyV8.debug()        
        
        if enable:            
            dbg.onDebugEvent = lambda type, evt: self.onDebugEvent(type, evt)
            dbg.onDebugMessage = lambda msg: self.onDebugMessage(msg)
        else:
            dbg.onDebugEvent = None
            dbg.onDebugMessage = None
            
        dbg.enabled = enable
            
    enabled = property(isEnabled, setEnabled)
        
    def onDebugMessage(self, msg):
        if self.onMessage:
            self.onMessage(msg)
            
    def onDebugEvent(self, type, evt):        
        if type == _PyV8.JSDebugEvent.Break:
            if self.onBreak: self.onBreak(JSDebug.BreakEvent(evt))
        elif type == _PyV8.JSDebugEvent.Exception:
            if self.onException: self.onException(JSDebug.ExceptionEvent(evt))
        elif type == _PyV8.JSDebugEvent.NewFunction:
            if self.onNewFunction: self.onNewFunction(JSDebug.NewFunctionEvent(evt))
        elif type == _PyV8.JSDebugEvent.BeforeCompile:
            if self.onBeforeCompile: self.onBeforeCompile(JSDebug.BeforeCompileEvent(evt))
        elif type == _PyV8.JSDebugEvent.AfterCompile:
            if self.onAfterCompile: self.onAfterCompile(JSDebug.AfterCompileEvent(evt))
        
debugger = JSDebug()

class JSEngine(_PyV8.JSEngine):
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        del self

class JSContext(_PyV8.JSContext):
    def __enter__(self):
        self.enter()
        
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        
        del self

import unittest
import logging

class TestContext(unittest.TestCase):
    def testMultiNamespace(self):
        self.assert_(not bool(JSContext.inContext))
        self.assert_(not bool(JSContext.entered))
        
        class Global(object):
            name = "global"
            
        g = Global()
        
        with JSContext(g) as ctxt:
            self.assert_(bool(JSContext.inContext))
            self.assertEquals(g.name, str(JSContext.entered.locals.name))
            self.assertEquals(g.name, str(JSContext.current.locals.name))
            
            class Local(object):
                name = "local"
                
            l = Local()
            
            with JSContext(l):
                self.assert_(bool(JSContext.inContext))
                self.assertEquals(l.name, str(JSContext.entered.locals.name))
                self.assertEquals(l.name, str(JSContext.current.locals.name))
            
            self.assert_(bool(JSContext.inContext))
            self.assertEquals(g.name, str(JSContext.entered.locals.name))
            self.assertEquals(g.name, str(JSContext.current.locals.name))

        self.assert_(not bool(JSContext.entered))
        self.assert_(not bool(JSContext.inContext))
        
    def testMultiContext(self):
        # Create an environment
        with JSContext() as ctxt0:
            ctxt0.securityToken = "password"
            
            global0 = ctxt0.locals
            global0.custom = 1234
                
            self.assertEquals(1234, int(global0.custom))
            
            # Create an independent environment
            with JSContext() as ctxt1:
                ctxt1.securityToken = ctxt0.securityToken
                 
                global1 = ctxt1.locals
                global1.custom = 1234
                
                self.assertEquals(1234, int(global0.custom))    
                self.assertEquals(1234, int(global1.custom))
                
                # Now create a new context with the old global
                with JSContext(global1) as ctxt2:
                    ctxt2.securityToken = ctxt1.securityToken

                    self.assertRaises(AttributeError, int, global1.custom)
                    self.assertRaises(AttributeError, int, global2.custom)
            
    def testSecurityChecks(self):
        with JSContext() as env1:
            env1.securityToken = "foo"
            
            # Create a function in env1.
            env1.eval("spy=function(){return spy;}")                

            spy = env1.locals.spy
            
            self.assert_(isinstance(spy, _PyV8.JSFunction))
            
            # Create another function accessing global objects.
            env1.eval("spy2=function(){return new this.Array();}")
            
            spy2 = env1.locals.spy2

            self.assert_(isinstance(spy2, _PyV8.JSFunction))
            
            # Switch to env2 in the same domain and invoke spy on env2.            
            env2 = JSContext()
            
            env2.securityToken = "foo"
            
            with env2:
                result = spy.apply(env2.locals)
                
                self.assert_(isinstance(result, _PyV8.JSFunction))
                
            env2.securityToken = "bar"
            
            # Call cross_domain_call, it should throw an exception
            with env2:
                self.assertRaises(UserWarning, spy2.apply, env2.locals)
                
    def testCrossDomainDelete(self):
        with JSContext() as env1:
            env2 = JSContext()
            
            # Set to the same domain.
            env1.securityToken = "foo"
            env2.securityToken = "foo"
            
            env1.locals.prop = 3
            env2.locals.env1 = env1.locals
            
            # Change env2 to a different domain and delete env1.prop.
            #env2.securityToken = "bar"
            
            with env2:
                self.assertEquals(3, int(env2.eval("env1.prop")))                
                self.assertEquals("false", str(e.eval("delete env1.prop")))
            
            # Check that env1.prop still exists.
            self.assertEquals(3, int(env1.locals.prop))            

class TestWrapper(unittest.TestCase):    
    def testConverter(self):
        with JSContext() as ctxt:
            ctxt.eval("""
                var_i = 1;
                var_f = 1.0;
                var_s = "test";
                var_b = true;
            """)
            
            vars = ctxt.locals 
            
            var_i = vars.var_i
            
            self.assert_(var_i)
            self.assertEquals(1, int(var_i))
            
            var_f = vars.var_f
            
            self.assert_(var_f)
            self.assertEquals(1.0, float(vars.var_f))
            
            var_s = vars.var_s
            self.assert_(var_s)
            self.assertEquals("test", str(vars.var_s))
            
            var_b = vars.var_b
            self.assert_(var_b)
            self.assert_(bool(var_b))
        
class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        with JSContext() as ctxt:
            self.assert_(str(JSEngine.version).startswith("1.0."))
        
    def testCompile(self):
        with JSContext() as ctxt:
            with JSEngine() as engine:
                s = engine.compile("1+2")
                
                self.assert_(isinstance(s, _PyV8.JSScript))
                
                self.assertEquals("1+2", s.source)
                self.assertEquals(3, int(s.run()))
            
    def testEval(self):
        with JSContext() as ctxt:
            self.assertEquals(3, int(ctxt.eval("1+2")))        
            
    def testGlobal(self):
        class Global(JSClass):
            version = "1.0"
            
        with JSContext(Global()) as ctxt:            
            vars = ctxt.locals
            
            # getter
            self.assertEquals(Global.version, str(vars.version))            
            self.assertEquals(Global.version, str(ctxt.eval("version")))
                        
            self.assertRaises(UserWarning, JSContext.eval, ctxt, "nonexists")
            
            # setter
            self.assertEquals(2.0, float(ctxt.eval("version = 2.0")))
            
            self.assertEquals(2.0, float(vars.version))       
            
    def testThis(self):
        class Global(JSClass): 
            version = 1.0            
                
        with JSContext(Global()) as ctxt:
            self.assertEquals("[object Global]", str(ctxt.eval("this")))
            
            self.assertEquals(1.0, float(ctxt.eval("this.version")))            
        
    def testObjectBuildInMethods(self):
        class Global(JSClass):
            version = 1.0
            
        with JSContext(Global()) as ctxt:            
            self.assertEquals("[object Global]", str(ctxt.eval("this.toString()")))
            self.assertEquals("[object Global]", str(ctxt.eval("this.toLocaleString()")))            
            self.assertEquals(Global.version, float(ctxt.eval("this.valueOf()").version))
            
            self.assert_(bool(ctxt.eval("this.hasOwnProperty(\"version\")")))
            
            ret = ctxt.eval("this.hasOwnProperty(\"nonexistent\")")
            
            self.assertEquals("valueOf", ret.valueOf.func_name)
            self.assertEquals(ret, ret.valueOf.func_owner)
            self.assertEquals("false", str(ret.valueOf()))
            
    def testPythonWrapper(self):
        class Global(JSClass):
            s = [1, 2, 3]
            
        g = Global()
        
        with JSContext(g) as ctxt:            
            ctxt.eval("""
                s[2] = s[1] + 2;
                s[0] = s[1];
                delete s[1];
            """)
            self.assertEquals([2, 4], g.s)
            
class TestDebug(unittest.TestCase):
    def setUp(self):
        self.engine = JSEngine()
        
    def tearDown(self):
        del self.engine
        
    events = []    
        
    def processDebugEvent(self, event):
        try:
            logging.debug("receive debug event: %s", repr(event))
            
            self.events.append(repr(event))
        except:
            logging.error(sys.exc_info())
        
    def testEventDispatch(self):        
        global debugger
        
        self.assert_(not debugger.enabled)
        
        debugger.onBreak = lambda evt: self.processDebugEvent(evt)
        debugger.onException = lambda evt: self.processDebugEvent(evt)
        debugger.onNewFunction = lambda evt: self.processDebugEvent(evt)
        debugger.onBeforeCompile = lambda evt: self.processDebugEvent(evt)
        debugger.onAfterCompile = lambda evt: self.processDebugEvent(evt)
        
        with JSContext() as ctxt:            
            debugger.enabled = True
            
            self.assertEquals(3, int(ctxt.eval("function test() { text = \"1+2\"; return eval(text) } test()")))
            
            debugger.enabled = False            
            
            self.assertRaises(UserWarning, JSContext.eval, ctxt, "throw 1")
            
            self.assert_(not debugger.enabled)                
            
        self.assertEquals(4, len(self.events))
        
if __name__ == '__main__':
    if "-v" in sys.argv:
        level = logging.DEBUG
    else:
        level = logging.WARN
    
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    
    logging.info("testing PyV8 module with V8 v%s", JSEngine.version)

    unittest.main()