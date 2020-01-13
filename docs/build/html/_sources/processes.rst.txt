.. _processes:

Processes
===============================

Process Definitions and Combinators
-----------------------------------

In the Beacon Calculus, components within a biological system are modelled as processes that can perform actions.  An action is an ordered pair that specifies the action name followed by the rate.  For example, we might define a process P as follows: ::

   P[] = {exampleAction, 5};

This process can perform a single action called ``exampleAction`` at rate ``5``, where rates are always the parameters of an exponential distribution.  Once this process performs ``exampleAction``, it cannot perform any other actions.  It is therefore said to be deadlocked and is removed from the system.

A process that can only perform one action isn't particularly useful, especially for biological systems.  We need a way to stitch series of actions together so that processes can perform complex behaviours.  We define the following three combinators:

* Prefix ``.``, where ``P[] = {a,ra}.{b,rb}`` is a process that performs action ``a`` at rate ``ra`` and then, once it has finished, performs action ``b`` at rate ``rb``.  Prefix is therefore used for actions that should happen in sequence.
* Choice ``+``, where ``P[] = {a,ra} + {b,rb}`` is a process that makes an exclusive choice between performing action ``a`` at rate ``ra`` and action ``b`` at rate ``rb``; it cannot perform both.  Note that the probability of picking action a is equal to ``ra/(ra+rb)``, so we can bias which outcome is more likely by scaling the actions' relative rates.
* Parallel ``||``, where ``P[] = {a,ra} || {b,rb}`` is a process where actions ``a`` and ``b`` are performed in parallel at their respective rates.

Using these three combinators, we can now define more complex processes.  In the following example, ::

   P[] = {a,ra}.{b,rb} || {c,rc}.{d,rd} + {e,re}.({f,rf} + g,rg});

we make an exclusive choice between actions ``c`` and ``e``.  If we pick ``c``, then we go on to perform action ``d``.  If we pick ``e``, then we make another choice between ``f`` and ``g``.  All the while, in parallel, we perform action ``a`` followed by action ``b``.

Parameters and Gates
--------------------

Oftentimes, processes need to keep track of certain quantities.  For example, if a process models the amount of a certain chemical reactant in a system, the process must be able to keep a count of how many molecules of this reactant are present over time.  If a process models a DNA replication fork, it has to keep track of where the replication fork is on the chromosome.  This is achieved through parameters, which are values that a process keeps track of.  Parameters are specified between square brackets, and processes can increase or decrease the value of their parameters over time.  They can also use the value of their parameters in the computation of rates.

Suppose there is a car which is at a particular location.  We can express this as ``Car[i]``, where the ``Car`` process has parameter ``i`` which specifies its location.  We can specify movement of the car through recursion.  The following process models a car that drives at rate 0.1, and increases its parameter value as it moves. ::

   Car[i] = {drive,0.1}.Car[i+1];

This car, as it is modelled above, will keep driving without stopping.  We may wish to specify, for example, that the car should stop when it reaches a bus stop at ``i=10``.  To express this, we use a gate: ::

   Car[i] = [i < 10] -> {drive,0.1}.Car[i+1];

The gate in front of the drive action specifies that this action can only be performed if the gate's condition is satisfied.  In this case, the value of the car's parameter ``i`` must be less than 10.  If the car starts at ``i=0``, then the car continues driving until ``i=10`` at which time the gate's condition is no longer satisfied.  The car can no longer perform any actions, so the process deadlocks and the simulation stops.

In addition to the less than comparison used here, bcs supports the following logical operators:

* ``<=``, less than or equal to,
* ``==``, equal to,
* ``!=``, is not equal to,
* ``>``, greater than,
* ``>=``, greater than or equal to,
* ``&``, logical and,
* ``|``, logical or, 
* ``~``, logical not.
