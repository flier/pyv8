.. _engine:

Javascript Engine
==========================

JSEngine
--------
.. autoclass:: PyV8.JSEngine
   :members: version, setFlags, dead, dispose,
             collect, idle, lowMemory, setMemoryLimit, setMemoryAllocationCallback,
             currentThreadId, terminateThread, terminateAllThreads

   .. py:method:: compile(source, name='', line=-1, col=-1, precompiled=None) -> JSScript object

      Compile the Javascript code to a :py:class:`PyV8.JSScript` object, which could be execute or visit it's AST.

      :param source: the Javascript code
      :type source: str or unicode
      :param str name: the name of the Javascript code
      :param integer line: the start line number of the Javascript code
      :param integer col: the start column number of the Javascript code
      :param buffer precompiled: the precompiled buffer of Javascript code
      :rtype: a compiled :py:class:`PyV8.JSScript` object

   .. py:method:: precompile(source) -> buffer object

      Precompile the Javascript code to an internal buffer, which could be used to improve the performance when compile the same script.

      :param source: the Javascript code
      :type source: str or unicode
      :rtype: a buffer object contains the precompiled internal data

.. autoclass:: PyV8.JSScript
   :members: source

   Compiled Javascript Code

   .. py::method:: run()
      Execute the compiled Javascript code

   .. py::method:: visit(handler)

.. autoclass:: PyV8.JSExtension
   :members: extensions, name, source, dependencies, autoEnable, registered

   .. py:method:: register()

.. toctree::
   :maxdepth: 2