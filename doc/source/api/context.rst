.. _context:
.. py:module:: PyV8

Javascript Context
==========================

JSContext
---------

.. autoclass:: JSContext
   :members:
   :inherited-members:
   :exclude-members: eval

   .. automethod:: eval(source, name='', line=-1, col=-1, precompiled=None) -> object:

      Execute the Javascript code in source and return the result

      :param source: the Javascript code
      :type source: str or unicode
      :param str name: the name of the Javascript code
      :param integer line: the start line number of the Javascript code
      :param integer col: the start column number of the Javascript code
      :param buffer precompiled: the precompiled buffer of Javascript code
      :rtype: the result

JSIsolate
---------

.. autoclass:: JSIsolate
   :members:
   :inherited-members:

JSExtension
-----------
.. autoclass:: JSExtension
   :members:
   :inherited-members:
   
.. toctree::
   :maxdepth: 2