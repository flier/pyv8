#!/usr/bin/env python
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
        setter = getattr(self, name).fset if hasattr(self, name) else None
        
        setattr(self, name, property(fget=getter, fset=setter))
    
    def __lookupGetter__(self, name):
        "Return the function bound as a getter to the specified property."
        return self.name.fget
    
    def __defineSetter__(self, name, setter):
        "Binds an object's property to a function to be called when an attempt is made to set that property."
        getter = getattr(self, name).fget if hasattr(self, name) else None
        
        setattr(self, name, property(fget=getter, fset=setter))
    
    def __lookupSetter__(self, name):
        "Return the function bound as a setter to the specified property."
        return self.name.fset

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
            
            self.assertEquals("false", str(ret.valueOf(ret)))
        finally:
            del engine
        
if __name__ == '__main__':
    unittest.main()