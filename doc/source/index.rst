.. PyV8 documentation master file, created by
   sphinx-quickstart on Wed Aug 31 11:19:01 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. _index:
.. py:module:: PyV8
.. testsetup:: *

   from PyV8 import *

Welcome to PyV8's documentation!
================================

`PyV8 <http://code.google.com/p/pyv8/>`_ is a python wrapper for the Google V8 engine [#f1]_,
it act as a bridge between the Python and JavaScript objects,
and support to host the Google's v8 engine in a python script.

You could create and enter a context to evaluate Javascript code in a few of code.

.. doctest::

   >>> ctxt = JSContext()               # create a context with an implicit global object
   >>> ctxt.enter()                     # enter the context (also support with statement)
   >>> ctxt.eval("1+2")                 # evalute the javascript expression and return a native python int
   3

You also could invoke the Javascript function from Python, or vice versa.

.. doctest::

    >>> class Global(JSClass):           # define a compatible javascript class
    ...   def hello(self):               # define a method
    ...     print "Hello World"
    ...
    >>> ctxt2 = JSContext(Global())      # create another context with the global object
    >>> ctxt2.enter()
    >>> ctxt2.eval("hello()")            # call the global object from javascript
    Hello World                          

.. toctree::
   :maxdepth: 2

   intro
   build
   1minute
   5minute
   samples
   api/api
   internal/internal
   faq

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

.. rubric:: Footnotes

.. [#f1] `Google V8 <http://code.google.com/apis/v8/>`_ is Google's open source JavaScript engine.

         V8 is written in C++ and is used in Google Chrome, the open source browser from Google.

         V8 implements ECMAScript as specified in ECMA-262, 3rd edition, and runs on Windows XP and Vista,
         Mac OS X 10.5 (Leopard), and Linux systems that use IA-32 or ARM processors.
         
         V8 can run standalone, or can be embedded into any C++ application.