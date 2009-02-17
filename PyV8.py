#!/usr/bin/env python
import _PyV8

Engine = _PyV8.Engine

import unittest

class TestWrapper(unittest.TestCase):
    def setUp(self):
        self.engine = Engine()
        
    def tearDown(self):
        del self.engine
        
    def testConverter(self):
        self.engine.eval("""
            var_i = 1;
            var_f = 1.0;
            var_s = "test";
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

class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        self.assertEquals("1.0.1", Engine.version)
        
    def testCompile(self):
        engine = Engine()
        
        try:
            s = engine.compile("1+2")
            
            self.assertEquals("1+2", s.source)
            self.assertEquals("3", s.run())
        finally:
            del engine
            
    def testExec(self):
        engine = Engine()
        
        try:
            self.assertEquals("3", engine.eval("1+2"))
        finally:
            del engine
            
    def testGlobal(self):
        class Global(object):
            version = "1.0"
            
        engine = Engine(Global())
        
        try:
            self.assertEquals(Global.version, str(engine.context.version))            
            self.assertEquals(Global.version, engine.eval("version"))
        finally:
            del engine

if __name__ == '__main__':
    unittest.main()