#!/usr/bin/env python
from __future__ import with_statement

import PyV8

class Global(PyV8.JSClass):
    def writeln(self, arg):
        print arg
        
with PyV8.JSContext(Global()) as ctxt:
    ctxt.eval("writeln('Hello World');")
