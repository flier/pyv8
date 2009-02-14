#!/usr/bin/env python
import _PyV8

Engine = _PyV8.Engine

import unittest

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
            
        context = Global()
        engine = Engine(context)
        
        try:
            self.assertEquals(Global.version, engine.eval("version"))
        finally:
            del engine

if __name__ == '__main__':
    unittest.main()