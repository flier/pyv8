#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement

import sys, os
import StringIO

import _PyV8

__all__ = ["JSError", "JSArray", "JSClass", "JSEngine", "JSContext", "JSLocker", "JSUnlocker", "debugger"]

class JSError(Exception):
    def __init__(self, impl):
        Exception.__init__(self)
        
        self._impl = impl
        
    def __str__(self):
        return str(self._impl)
        
    def __getattribute__(self, attr):
        impl = super(JSError, self).__getattribute__("_impl")
        
        try:
            return getattr(impl, attr)
        except AttributeError:
            return super(JSError, self).__getattribute__(attr)
            
_PyV8._JSError._jsclass = JSError

JSArray = _PyV8.JSArray

class JSLocker(_PyV8.JSLocker):
    def __enter__(self):
        self.enter()
        
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        
    def __nonzero__(self):
        return self.entered()

class JSUnlocker(_PyV8.JSUnlocker):
    def __enter__(self):
        self.enter()
        
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

    def __nonzero__(self):
        return self.entered()
        
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
            return JSDebug.Frame(self.state.frame(idx))
        
        @property    
        def selectedFrame(self):
            return int(self.state.selectedFrame())
        
        @property
        def frames(self):
            return JSDebug.Frames(self)
        
        def __repr__(self):
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
        def __init__(self, script):            
            self.script = script            
            
        @property
        def source(self):
            return self.script.source()
            
        @property
        def id(self):
            return self.script.id()
            
        @property
        def name(self):
            return self.script.name()
            
        @property
        def lineOffset(self):
            return self.script.lineOffset() 
            
        @property
        def lineCount(self):
            return self.script.lineCount()
        
        @property    
        def columnOffset(self):
            return self.script.columnOffset()
            
        @property
        def type(self):
            return self.script.type()
            
        def __repr__(self):
            return "<%s script %s @ %d:%d> : '%s'" % (self.type, self.name,
                                                      self.lineOffset, self.columnOffset,
                                                      self.source)
            
    class CompileEvent(StateEvent):
        def __init__(self, event):
            self.event = event

        @property
        def script(self):
            if not hasattr(self, "_script"):
                setattr(self, "_script", JSDebug.Script(self.event.script()))
            
            return self._script
        
        def __str__(self):
            return str(self.script)
            
    class BeforeCompileEvent(CompileEvent):
        type = _PyV8.JSDebugEvent.BeforeCompile        
        
        def __init__(self, event):            
            JSDebug.CompileEvent.__init__(self, event)
        
        def __repr__(self):
            return "before compile script: %s\n%s" % (repr(self.script), repr(self.state))
    
    class AfterCompileEvent(CompileEvent):
        type = _PyV8.JSDebugEvent.AfterCompile        

        def __init__(self, event):            
            JSDebug.CompileEvent.__init__(self, event)

        def __repr__(self):
            return "after compile script: %s\n%s" % (repr(self.script), repr(self.state))

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
    def __init__(self, obj=None):
        if JSLocker.actived:
            self.lock = JSLocker()
            self.lock.enter()
            
        _PyV8.JSContext.__init__(self, obj)
        
    def __enter__(self):
        self.enter()
        
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        
        if hasattr(JSLocker, 'lock'):
            self.lock.leave()
            self.lock = None
            
        del self
        
# contribute by marc boeker <http://code.google.com/u/marc.boeker/>
def convert(obj):    
    if type(obj) == _PyV8.JSArray:
        return [convert(v) for v in obj]
    
    if type(obj) == _PyV8.JSObject:
        return dict([[k, convert(obj.__getattr__(k))] for k in obj.__members__])
        
    return obj

import datetime
import unittest
import logging
import traceback

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
        
    def _testMultiContext(self):
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
            
    def _testSecurityChecks(self):
        with JSContext() as env1:
            env1.securityToken = "foo"
            
            # Create a function in env1.
            env1.eval("spy=function(){return spy;}")                

            spy = env1.locals.spy
            
            self.assert_(isinstance(spy, _PyV8.JSFunction))
            
            # Create another function accessing global objects.
            env1.eval("spy2=function(){return 123;}")
            
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
                self.assertRaises(JSError, spy2.apply, env2.locals)
                
    def _testCrossDomainDelete(self):
        with JSContext() as env1:
            env2 = JSContext()
            
            # Set to the same domain.
            env1.securityToken = "foo"
            env2.securityToken = "foo"
            
            env1.locals.prop = 3
            
            env2.locals.env1 = env1.locals
            
            # Change env2 to a different domain and delete env1.prop.
            #env2.securityToken = "bar"
            
            self.assertEquals(3, int(env1.eval("prop")))
            
            print env1.eval("env1")
            
            with env2:
                self.assertEquals(3, int(env2.eval("this.env1.prop")))
                self.assertEquals("false", str(e.eval("delete env1.prop")))
            
            # Check that env1.prop still exists.
            self.assertEquals(3, int(env1.locals.prop))            

class TestWrapper(unittest.TestCase):    
    def testAutoConverter(self):
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
            
            attrs = dir(ctxt.locals)
                        
            self.assert_(attrs)
            self.assert_("var_i" in attrs)
            self.assert_("var_f" in attrs)
            self.assert_("var_s" in attrs)
            self.assert_("var_b" in attrs)
            
    def testExactConverter(self):
        class MyInteger(int, JSClass):
            pass
        
        class MyString(str, JSClass):
            pass
        
        class MyUnicode(unicode, JSClass):
            pass
        
        class MyDateTime(datetime.time, JSClass):
            pass
        
        class Global(JSClass):
            var_bool = True
            var_int = 1
            var_float = 1.0
            var_str = 'str'
            var_unicode = u'unicode'
            var_datetime = datetime.datetime.now()
            var_date = datetime.date.today()
            var_time = datetime.time()
            
            var_myint = MyInteger()
            var_mystr = MyString('mystr')
            var_myunicode = MyUnicode('myunicode')
            var_mytime = MyDateTime()
            
        with JSContext(Global()) as ctxt:
            typename = ctxt.eval("function (name) { return this[name].constructor.name; }")
            typeof = ctxt.eval("function (name) { return typeof(this[name]); }")
            
            self.assertEquals('Boolean', typename('var_bool'))
            self.assertEquals('Number', typename('var_int'))
            self.assertEquals('Number', typename('var_float'))
            self.assertEquals('String', typename('var_str'))
            self.assertEquals('String', typename('var_unicode'))
            self.assertEquals('Date', typename('var_datetime'))
            self.assertEquals('Date', typename('var_date'))
            self.assertEquals('Date', typename('var_time'))            
            
            # TODO: fill the constructor name of python object
            self.assertEquals('', typename('var_myint'))
            self.assertEquals('', typename('var_mystr'))
            self.assertEquals('', typename('var_myunicode'))
            self.assertEquals('', typename('var_mytime'))            
            
            self.assertEquals('object', typeof('var_myint'))
            self.assertEquals('object', typeof('var_mystr'))
            self.assertEquals('object', typeof('var_myunicode'))
            self.assertEquals('object', typeof('var_mytime'))  
            
    def testFunction(self):
        with JSContext() as ctxt:
            func = ctxt.eval("""
                function()
                {
                    function a()
                    {
                        return "abc";    
                    }
                
                    return a();    
                };
                """)
            
            self.assertEquals("abc", str(func()))
            self.assert_(func != None)
            self.assertFalse(func == None)
        
    def testJSError(self):
        with JSContext() as ctxt:
            try:
                ctxt.eval('throw "test"')
                self.fail()
            except:
                self.assert_(JSError, sys.exc_type)
                
    def testErrorInfo(self):
        with JSContext() as ctxt:
            with JSEngine() as engine:
                try:
                    engine.compile("""
                        function hello()
                        {
                            throw Error("hello world");
                        }
                        
                        hello();""", "test", 10, 10).run()
                    self.fail()
                except JSError, e:
                    self.assert_(str(e).startswith('JSError: Error: hello world ( test @ 14 : 34 )  ->'))
                    self.assertEqual("Error", e.name)
                    self.assertEqual("hello world", e.message)
                    self.assertEqual("test", e.scriptName)
                    self.assertEqual(14, e.lineNum)
                    self.assertEqual(102, e.startPos)
                    self.assertEqual(103, e.endPos)
                    self.assertEqual(34, e.startCol)
                    self.assertEqual(35, e.endCol)
                    self.assertEqual('throw Error("hello world");', e.sourceLine.strip())
                    self.assertEqual('Error: hello world\n' +
                                     '    at Error (unknown source)\n' +
                                     '    at hello (test:14:34)\n' +
                                     '    at test:17:24', e.stackTrace)
        
    def testPythonException(self):
        class Global(JSClass):
            def raiseException(self):
                raise RuntimeError("Hello")
                
        with JSContext(Global()) as ctxt:
            r = ctxt.eval("""
                msg ="";
                try
                {
                    this.raiseException()
                }
                catch(e)
                {
                    msg += "catch " + e + ";";
                }
                finally
                {
                    msg += "finally";
                }""")
            self.assertEqual("catch Error: Hello;finally", str(ctxt.locals.msg))
        
    def testExceptionMapping(self):
        class Global(JSClass):
            def raiseIndexError(self):
                return [1, 2, 3][5]
            
            def raiseAttributeError(self):
                None.hello()
            
            def raiseSyntaxError(self):
                eval("???")
            
            def raiseTypeError(self):
                int(sys)
                
            def raiseNotImplementedError(self):
                raise NotImplementedError()
        
        with JSContext(Global()) as ctxt:
            ctxt.eval("try { this.raiseIndexError(); } catch (e) { msg = e; }")
            
            self.assertEqual("RangeError", str(ctxt.locals.msg))
            
            ctxt.eval("try { this.raiseAttributeError(); } catch (e) { msg = e; }")
            
            self.assertEqual("ReferenceError", str(ctxt.locals.msg))
            
            ctxt.eval("try { this.raiseSyntaxError(); } catch (e) { msg = e; }")
            
            self.assertEqual("SyntaxError", str(ctxt.locals.msg))
            
            ctxt.eval("try { this.raiseTypeError(); } catch (e) { msg = e; }")
            
            self.assertEqual("TypeError", str(ctxt.locals.msg))
            
            ctxt.eval("try { this.raiseNotImplementedError(); } catch (e) { msg = e; }")
            
            self.assertEqual("Error", str(ctxt.locals.msg))
    def testArray(self):
        with JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();
                
                for (i=0; i<10; i++)
                {
                    array[i] = 10-i;
                }
                
                array;
                """)
            
            self.assert_(isinstance(array, _PyV8.JSArray))
            self.assertEqual(10, len(array))
            
            self.assert_(5 in array)
            self.assertFalse(15 in array)
            
            l = list(array)
            
            self.assertEqual(10, len(l))            
            
            for i in xrange(10):
                self.assertEqual(10-i, array[i])
                self.assertEqual(10-i, l[i])
            
            array[5] = 0
            
            self.assertEqual(0, array[5])
            
            del array[5]
            
            self.assertRaises(IndexError, lambda: array[5])
            
            ctxt.locals.array1 = JSArray(5)
            ctxt.locals.array2 = JSArray([1, 2, 3, 4, 5])
            
            for i in xrange(len(ctxt.locals.array2)):
                ctxt.locals.array1[i] = ctxt.locals.array2[i] * 10
                            
            ctxt.eval("""
                var sum = 0;
                
                for (i=0; i<array1.length; i++)
                    sum += array1[i]
                
                for (i=0; i<array2.length; i++)
                    sum += array2[i]                
                """)
            
            self.assertEqual(165, ctxt.locals.sum)
            
    def testForEach(self):
        class NamedClass(JSClass):
            foo = 1
            
            def __init__(self):
                self.bar = 2
                
        with JSContext() as ctxt:
            func = ctxt.eval("""function (k) {
                var result = [];    
                for (var prop in k) {
                  result.push(prop);
                }
                return result;
            }""")
            
            self.assertEquals(["bar"], list(func(NamedClass())))
            self.assertEquals(["0", "1", "2"], list(func([1, 2, 3])))
            
    def testDict(self):
        import UserDict
        
        with JSContext() as ctxt:
            obj = ctxt.eval("var r = { 'a' : 1, 'b' : 2 }; r")
            
            self.assertEqual(1, obj.a)
            self.assertEqual(2, obj.b)
            
            self.assertEqual({ 'a' : 1, 'b' : 2 }, dict(obj))            
        
            self.assertEqual({ 'a': 1,
                               'b': [1, 2, 3],
                               'c': { 'str' : 'goofy',
                                      'float' : 1.234,
                                      'obj' : { 'name': 'john doe' }},                                      
                               'd': True,
                               'e': None },
                             convert(ctxt.eval("""var x =
                             { a: 1,
                               b: [1, 2, 3],
                               c: { str: 'goofy',
                                    float: 1.234,
                                    obj: { name: 'john doe' }},
                               d: true,
                               e: null }; x""")))
        
    def testDate(self):
        with JSContext() as ctxt:            
            now1 = ctxt.eval("new Date();")
            
            self.assert_(now1)
            
            now2 = datetime.datetime.utcnow()
            
            delta = now2 - now1 if now2 > now1 else now1 - now2
            
            self.assert_(delta < datetime.timedelta(seconds=1))
            
            func = ctxt.eval("function (d) { return d.toString(); }")
            
            now = datetime.datetime.now() 
            
            self.assert_(str(func(now)).startswith(now.strftime("%a %b %d %Y %H:%M:%S")))
    
    def testUnicode(self):
        with JSContext() as ctxt:
            self.assertEquals(u"人", unicode(ctxt.eval("\"人\""), "utf-8"))
            self.assertEquals(u"é", unicode(ctxt.eval("\"é\""), "utf-8"))
            
            func = ctxt.eval("function (msg) { return msg.length; }")
            
            self.assertEquals(2, func(u"测试"))
            
    def testClassicStyleObject(self):
        class FileSystemWarpper:
            @property
            def cwd(self):
                return os.getcwd() 
            
        class Global:
            @property
            def fs(self):
                return FileSystemWarpper()
                
        with JSContext(Global()) as ctxt:    
            self.assertEquals(os.getcwd(), ctxt.eval("fs.cwd"))
            
class TestMutithread(unittest.TestCase):
    def testLocker(self):
        self.assertFalse(JSLocker.actived)
        self.assertFalse(JSLocker.locked)
        with JSContext() as ctxt:
            with JSLocker() as outter_locker:        
                self.assertTrue(JSLocker.actived)
                self.assertTrue(JSLocker.locked)
                
                self.assertTrue(outter_locker)
                
                with JSLocker() as inner_locker:
                    self.assertTrue(JSLocker.locked)
                    
                    self.assertTrue(outter_locker)
                    self.assertTrue(inner_locker)
                    
                    with JSUnlocker() as unlocker:
                        self.assertFalse(JSLocker.locked)
                    
                        self.assertTrue(outter_locker)
                        self.assertTrue(inner_locker)
                        
                    self.assertTrue(JSLocker.locked)
                    
        self.assertTrue(JSLocker.actived)
        self.assertFalse(JSLocker.locked)        
        
class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        with JSContext() as ctxt:
            self.assert_(str(JSEngine.version).startswith("1."))
            self.assertFalse(JSEngine.dead)
        
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
                        
            self.assertRaises(JSError, JSContext.eval, ctxt, "nonexists")
            
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
            
            self.assertFalse(ctxt.eval("this.hasOwnProperty(\"nonexistent\")"))
            
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
            logging.error("fail to process debug event")            
            logging.debug(traceback.extract_stack())
        
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
            
            self.assertRaises(JSError, JSContext.eval, ctxt, "throw 1")
            
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