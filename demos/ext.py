#!/usr/bin/env python
from PyV8 import *

firstSrc = "function hello(s) { return 'hello ' + s; }"
firstPy = JSExtension("hello/javascript", firstSrc, register=False)
firstPy.register()


secondSrc = "native function title(s);"
secondPy = JSExtension("title/python", secondSrc, lambda secondfunc: lambda name: "Mr. " + name, register=False)
secondPy.register()

with JSContext(extensions=['title/python', 'hello/javascript']) as ctx:
	print ctx.eval("hello(title('flier'))")
