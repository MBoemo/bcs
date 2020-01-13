.. _simulation:

Simulation
===============================

Options
-------

The bcs executable accepts a number of arguments to control the simulation of a Beacon Calculus model:

* ``-t``, the number of threads. Simulations can be run independently on separate threads, so multithreading can speed up runtimes considerably. We recommend using as many threads as you have available if the simulation is large.
* ``-m``, the maximum number of actions allowed before the simulation is stopped. If ``-m 100`` is specified, the simulation will stop (even if it is not deadlocked) after a total of 100 actions have been performed by processes in the system. In practice, this is useful for checking a model's behaviour.
* ``-d``, time at which the simulation stops. If ``-d 60`` is specified, the simulation will end when the time is equal to 60, or before if the system has deadlocked.

Algorithm
---------

Models are simulated using a modified version of the `Gillespie algorithm <https://en.wikipedia.org/wiki/Gillespie_algorithm>`_, and are therefore subject to some of the algorithm's disadvantages.  In particular, systems with long simulation durations and lots of high-rate actions can head to slow bcs runtimes.  Ways to improve this are currently in development.

Casting
-------

As a simulation runs, arithmetic functions can be applied to parameters, channel names, and sent/received values. Each of these can be either ints or floats. When a function acts on both ints and floats, the result is upcast to a float.  For example in the following bcs code, process ``P`` starts with ``i=0`` (an int) but after it is multiplied by 2.0 in the first recursion, the type of ``i`` is a float thereafter. ::

   //process definition
   P[i] = {multiplyParameter,1.0}.P[i*2.0];

   //system line
   P[0];

In any operation where comparisons must be made (gates, handshakes, and beacons) values must be ints, and bcs will throw an error if passed a float. It is perfectly acceptable to use a float as a parameter, where it may be used to scale an action rate, etc.

Output
------

Simulation outputs are always written in a file with the ``.simulation.bcs`` extension and the prefix specified by the user using the ``-o`` flag. Each simulation begins with the line ``>=======``.  Each line is an action that was performed by a process in the system, and the tab-delimited columns, from left to right, specify:

* the time in the simulation when the action happened,
* the name of the action that was performed (or the channel name, if it was a beacon or handshake action),
* the name of the process that performed the action,
* the parameter values (if any) of that process at the time when the action was performed.

For example, the output line ::

      0.315113	act1	P	i	2	j	5

indicates that an action named ``act1`` was performed by process ``P`` at time 0.315113.  Process ``P`` has two parameters, ``i`` and ``j``, and when this action was performed, ``i=2`` and ``j=5``.

See :ref:`quickstart` for a further example of the output file format.

