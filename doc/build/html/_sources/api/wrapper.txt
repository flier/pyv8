.. _wrapper:
.. py:module:: PyV8

.. testsetup:: *

   from PyV8 import *

Interoperability
================

As the most basic and important design principle, the PyV8 user doesn't need pay additional efforts for interoperability. It means that your Python code could seamless access the property or call the function of Javascript code, or vice versa. PyV8 will do the dirty work under the table, such as :ref:`typeconv`, :ref:`funcall` and :ref:`exctrans`.

.. _typeconv:

Type Conversion
---------------

Python and Javascript has different type system and build-in primitives. PyV8 have to guess the conversion by the type of source value.

When convert Python value to Javascript value, PyV8 will try the following rules first. The remaining unknown Python type will be convert to a plain Javascript object.

=============================   ===============     ===============     ================
Python Type                     Python Value        Javascript Type     Javascript Value
=============================   ===============     ===============     ================
:py:class:`NoneType`            None                Null [#f1]_         null
:py:func:`bool`                 True/False          Boolean [#f2]_      true/false
:py:func:`int`                  123                 Number [#f3]_       123
:py:func:`long`                 123                 Number              123
:py:func:`float`                3.14                Number              3.14
:py:func:`str`                  'test'              String [#f4]_       'test'
:py:func:`unicode`              u'test'             String              'test'
:py:class:`datetime.datetime`                       Date [#f5]_
:py:class:`datetime.time`                           Date
builtin_function_or_method                          Function
:py:func:`type`                                     Function
=============================   ===============     ===============     ================

.. note::

    All the Python *function*, *method* and *type* will be convert to a Javascript function, because even the Python *type* could be used as a constructor and create a new instance.

On the other hand, PyV8 will try the following rules to convert Javascript value to Python value. 

===============     ================    =============================   ============
Javascript Type     Javascript Value    Python Type                     Python Value
===============     ================    =============================   ============
Null                null                :py:class:`NoneType`            None
Undefined           undefined           :py:class:`NoneType`            None
Boolean             true/false          :py:func:`bool`                 True/False
String              'test'              :py:func:`str`                  'test'
Number/Int32        123                 :py:func:`int`                  123
Number              3.14                :py:func:`float`                3.14
Date                                    :py:class:`datetime.datetime`
Array [#f6]_                            :py:class:`JSArray`
Function                                :py:class:`JSFunction`
Object                                  :py:class:`JSObject`
===============     ================    =============================   ============

.. note::

    Even ECMAScript standard has only one Number type for both integer and float number, Google V8 defined the standalone Integer/Int32/Uint32 class to improve the performance. PyV8 use the internal V8 type system to do the same job.

.. _funcall:

Function Call
-------------

.. _exctrans:

Exception Translation
---------------------

JSClass
-------

.. autoclass:: JSClass
   :members:
   :inherited-members:

JSObject
--------

.. autoclass:: JSObject
   :members:
   :inherited-members:

   .. automethod:: __getattr__(name) -> object

        Called when an attribute lookup has not found the attribute in the usual places (i.e. it is not an instance attribute nor is it found in the class tree for self). name is the attribute name. This method should return the (computed) attribute value or raise an AttributeError exception.

        .. seealso:: :py:meth:`object.__getattr__`

   .. automethod:: __setattr__(name, value) -> None

        Called when an attribute assignment is attempted. This is called instead of the normal mechanism (i.e. store the value in the instance dictionary). name is the attribute name, value is the value to be assigned to it.

        .. seealso:: :py:meth:`object.__setattr__`

   .. automethod:: __delattr__(name) -> None

        Like __setattr__() but for attribute deletion instead of assignment. This should only be implemented if del obj.name is meaningful for the object.

        .. seealso:: :py:meth:`object.__delattr__`

   .. automethod:: __hash__() -> int

        Called by built-in function hash() and for operations on members of hashed collections including set, frozenset, and dict. __hash__() should return an integer. The only required property is that objects which compare equal have the same hash value; it is advised to somehow mix together (e.g. using exclusive or) the hash values for the components of the object that also play a part in comparison of objects.

        .. seealso:: :py:meth:`object.__hash__`

   .. autoattribute:: __members__

        Deprecated, use the :py:meth:`JSObject.keys` to get a list of an object’s attributes.

        .. seealso:: :py:attr:`object.__members__`

   .. automethod:: __getitem__(key) -> object

        Called to implement evaluation of self[key]. For sequence types, the accepted keys should be integers and slice objects. Note that the special interpretation of negative indexes (if the class wishes to emulate a sequence type) is up to the __getitem__() method. If key is of an inappropriate type, TypeError may be raised; if of a value outside the set of indexes for the sequence (after any special interpretation of negative values), IndexError should be raised. For mapping types, if key is missing (not in the container), KeyError should be raised.

        .. seealso:: :py:meth:`object.__getitem__`
   
   .. automethod:: __setitem__(key, value) -> None
   
        Called to implement assignment to self[key]. Same note as for __getitem__(). This should only be implemented for mappings if the objects support changes to the values for keys, or if new keys can be added, or for sequences if elements can be replaced. The same exceptions should be raised for improper key values as for the __getitem__() method.

        .. seealso:: :py:meth:`object.__setitem__`

   .. automethod:: __delitem__(key) -> None

        Called to implement deletion of self[key]. Same note as for __getitem__(). This should only be implemented for mappings if the objects support removal of keys, or for sequences if elements can be removed from the sequence. The same exceptions should be raised for improper key values as for the __getitem__() method.

        .. seealso:: :py:meth:`object.__delitem__`

   .. automethod:: __contains__(key) -> bool

        Called to implement membership test operators. Should return true if item is in self, false otherwise. For mapping objects, this should consider the keys of the mapping rather than the values or the key-item pairs.

        .. seealso:: :py:meth:`object.__contains__`

   .. automethod:: __nonzero__() -> bool

        Called to implement truth value testing and the built-in operation bool(); should return False or True, or their integer equivalents 0 or 1. When this method is not defined, __len__() is called, if it is defined, and the object is considered true if its result is nonzero. If a class defines neither __len__() nor __nonzero__(), all its instances are considered true.

        .. seealso:: :py:meth:`object.__nonzero__`

   .. automethod:: __eq__(other) -> bool

        .. seealso:: :py:meth:`object.__eq__`

   .. automethod:: __ne__(other) -> bool

        .. seealso:: :py:meth:`object.__ne__`

   .. automethod:: __int__() -> int

        .. seealso:: :py:meth:`object.__int__`

   .. automethod:: __float__() -> float

        .. seealso:: :py:meth:`object.__float__`

   .. automethod:: __str__() -> str

        .. seealso:: :py:meth:`object.__str__`


JSArray
-------

.. autoclass:: JSArray
   :members:
   :inherited-members:

   .. automethod:: __init__(len) -> JSArray object
      :param int len: the array size

   .. automethod:: __init__(items) -> JSArray object
      :param list items: the item list

   .. automethod:: __len__() -> int

       Called to implement the built-in function len(). Should return the length of the object, an integer >= 0. Also, an object that doesn’t define a __nonzero__() method and whose __len__() method returns zero is considered to be false in a Boolean context.

       .. seealso:: :py:meth:`object.__len__`

   .. automethod:: __iter__() -> iterator

       This method is called when an iterator is required for a container. This method should return a new iterator object that can iterate over all the objects in the container. For mappings, it should iterate over the keys of the container, and should also be made available as the method iterkeys().

       .. seealso:: :py:meth:`object.__iter__`

JSFunction
----------

.. autoclass:: JSFunction
   :members:
   :inherited-members:

   .. automethod:: __call__() -> iterator

       Called when the instance is “called” as a function; if this method is defined, x(arg1, arg2, ...) is a shorthand for x.__call__(arg1, arg2, ...).

       .. seealso:: :py:meth:`object.__call__`

.. toctree::
   :maxdepth: 2

.. rubric:: Footnotes

.. [#f1] **Null Type**

         The type Null has exactly one value, called null.

.. [#f2] **Boolean Type**

         The type Boolean represents a logical entity and consists of exactly two unique values. One is called true and the other is called false.

.. [#f3] **Number Type**

        The type **Number** is a set of values representing numbers. In ECMAScript, the set of values represents the double-precision 64-bit format IEEE 754 values including the special “Not-a-Number” (NaN) values, positive infinity, and negative infinity.

.. [#f4] **String Type**

        The type String is the set of all string values. A string value is a finite ordered sequence of zero or more 16-bit unsigned integer values.

.. [#f5] **Date Object**

        A Date object contains a number indicating a particular instant in time to within a millisecond. The number may also be NaN, indicating that the Date object does not represent a specific instant of time.

.. [#f6] **Array Object**

        Array objects give special treatment to a certain class of property names. A property name P (in the form of a string value) is an array index if and only if ToString(ToUint32(P)) is equal to P and ToUint32(P) is not equal to 232−1.