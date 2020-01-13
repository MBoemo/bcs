.. _code_layout:

Formatting
===============================

A bcs model, from top to bottom, consists of three sections:

* definition of static variables,
* definition of processes,
* system line (that is, the initial processes that are in the system at t=0).

In bcs, lines must be terminated by semicolons and any whitespace/newlines are ignored.  If we want to define some variable x that will be used by a process, the three lines: ::

   x = 5;
   x                 = 5;
   x
   =
   5;

are all acceptable.  Variables may be assigned either a float or an int, so assigning: ::

   x = 3.452;

is certainly acceptable.  Variable names must either lead with an underscore or a letter.  Thereafter, they can contain letters and numbers.  However, leading with a number is not acceptable.  The following variable names are valid: ::

   myVar
   _myVar
   __myVar
   m1yVar99

But a variable name such as ``99myVar`` is not.

Comments are specified by ``//`` which tell the bcs parser that the rest of the line should be ignored.
