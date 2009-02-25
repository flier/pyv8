#!/usr/bin/env python
import sys
import _PyV8

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
    class DebugEvent(object):
        def __init__(self, type, event):            
            self.type = type
            self.event = event
            
    class BreakEvent(DebugEvent):
        def __init__(self, event):
            JSDebug.DebugEvent.__init__(self, _PyV8.JSDebugEvent.Break, event)
            
    class ExceptionEvent(DebugEvent):
        def __init__(self, event):
            JSDebug.DebugEvent.__init__(self, _PyV8.JSDebugEvent.Exception, event)

    class NewFunctionEvent(DebugEvent):
        def __init__(self, event):
            JSDebug.DebugEvent.__init__(self, _PyV8.JSDebugEvent.NewFunction, event)
    
    class CompileEvent(DebugEvent):
        def __init__(self, type, event):
            JSDebug.DebugEvent.__init__(self, type, event)
            
        @property
        def source(self):
            return str(self.event.script())
            
        def __str__(self):
            return self.source
            
    class BeforeCompileEvent(CompileEvent):
        def __init__(self, event):
            JSDebug.CompileEvent.__init__(self, _PyV8.JSDebugEvent.BeforeCompile, event)
        
        def __repr__(self):
            return "before compile script: " + self.source
    
    class AfterCompileEvent(CompileEvent):
        def __init__(self, event):
            JSDebug.CompileEvent.__init__(self, _PyV8.JSDebugEvent.AfterCompile, event)

        def __repr__(self):
            return "after compile script: " + self.source

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

JSEngine = _PyV8.JSEngine

import unittest

class TestWrapper(unittest.TestCase):
    def setUp(self):
        self.engine = JSEngine()
        
    def tearDown(self):
        del self.engine        
        
    def testConverter(self):
        self.engine.eval("""
            var_i = 1;
            var_f = 1.0;
            var_s = "test";
            var_b = true;
        """)
        
        var_i = self.engine.context.var_i
        
        self.assert_(var_i)
        self.assertEquals(1, int(var_i))
        
        var_f = self.engine.context.var_f
        
        self.assert_(var_f)
        self.assertEquals(1.0, float(self.engine.context.var_f))
        
        var_s = self.engine.context.var_s
        self.assert_(var_s)
        self.assertEquals("test", str(self.engine.context.var_s))
        
        var_b = self.engine.context.var_b
        self.assert_(var_b)
        self.assert_(bool(var_b))

class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        self.assertEquals("1.0.1", JSEngine.version)
        
    def testCompile(self):
        engine = JSEngine()
        
        try:
            s = engine.compile("1+2")
            
            self.assertEquals("1+2", s.source)
            self.assertEquals(3, int(s.run()))
        finally:
            del engine
            
    def testExec(self):
        engine = JSEngine()
        
        try:
            self.assertEquals(3, int(engine.eval("1+2")))
        finally:
            del engine
            
    def testGlobal(self):
        class Global(object):
            version = "1.0"
            
        engine = JSEngine(Global())
        
        try:
            # getter
            self.assertEquals(Global.version, str(engine.context.version))            
            self.assertEquals(Global.version, str(engine.eval("version")))
                        
            self.assertRaises(UserWarning, JSEngine.eval, engine, "nonexists")
            
            # setter
            self.assertEquals(2.0, float(engine.eval("version = 2.0")))
            
            self.assertEquals(2.0, float(engine.context.version))       
        finally:
            del engine
            
    def testThis(self):
        class Global(object): 
            version = 1.0
        
        engine = JSEngine(Global())
        
        try:        
            self.assertEquals("[object global]", str(engine.eval("this")))
            
            self.assertEquals(1.0, float(engine.eval("this.version")))
        finally:
            del engine
        
    def testObjectBuildInMethods(self):
        class Global(JSClass):
            version = 1.0
        
        engine = JSEngine(Global())
        
        try:
            self.assertEquals("[object Global]", str(engine.eval("this.toString()")))
            self.assertEquals("[object Global]", str(engine.eval("this.toLocaleString()")))
            self.assertEquals(Global.version, float(engine.eval("this.valueOf()").version))
            self.assert_(bool(engine.eval("this.hasOwnProperty(\"version\")")))
            
            ret = engine.eval("this.hasOwnProperty(\"nonexistent\")")
            
            self.assertEquals("valueOf", ret.valueOf.func_name)
            self.assertEquals(ret, ret.valueOf.func_owner)
            self.assertEquals("false", str(ret.valueOf()))
        finally:
            del engine
            
class TestDebug(unittest.TestCase):
    def setUp(self):
        self.engine = JSEngine()
        
    def tearDown(self):
        del self.engine        
        
    def testEventDispatch(self):        
        global debugger
        
        self.assert_(not debugger.enabled)
        
        events = []        
        
        debugger.onBreak = lambda evt: events.append(repr(evt))
        debugger.onException = lambda evt: events.append(repr(evt))
        debugger.onNewFunction = lambda evt: events.append(repr(evt))
        debugger.onBeforeCompile = lambda evt: events.append(repr(evt))
        debugger.onAfterCompile = lambda evt: events.append(repr(evt))
        
        debugger.enabled = True
        
        engine = JSEngine()
        
        try:
            engine.eval("eval(\"1+2\")") 
        finally:
            del engine
            
            debugger.enabled = False
            
        self.assert_(not debugger.enabled)                
        
if __name__ == '__main__':
    unittest.main()