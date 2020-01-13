.. _plotting:

Plotting
===============================

During the process of developing bcs and applying it to biological problems, we found that in practice, the type of plots required for a given application were often nonstandard and more complicated than number-of-processes-over-time plots. Indeed, all of the examples in `Beacon Calculus manuscript <https://www.biorxiv.org/content/10.1101/579029v2>`_ required bespoke plots that were typical of the applications' respective fields.  For this reason, we focused mainly on making an output that was easy to parse (see :ref:`simulation`) so that users could reshape the results into whatever plot was appropriate.

For completeness, testing, and simper applications, we included a plotting script bcs/utils/plot_bcs.py which uses matplotlib to plot the value of a process's parameter over time. Suppose we have the following model: ::

   r = 1.0;
   fast = 1000;

   A[count] = {@react![0],r*count};
   B[count] = {@react?[0],count}.{@gain![0],fast};
   C[count] = {@gain?[0],fast}.C[count+1];

   A[10] || B[10] || C[0];

Here, processes ``A``, ``B``, and ``C`` represent populations of distinct chemical species, where the molecules of each species is given by parameter ``count``.  ``A`` and ``B`` both start with populations of 10, and ``A`` and ``B`` can react at rate ``r`` to add to the population of ``C``.  We can save the above model as simple.bc can run five simulations of the model using bcs: ::

   bcs -s 5 -t 1 -o abcsim simple.bc

which creates a file abcsim.simulation.bcs.  We can plot the value of ``i`` in ``C`` over time: ::

   python plot_bcs.py -a gain -p C -i count -o myplot.png

The resulting plot file ``myplot.png`` will have five traces, one for each of the five simulations run, showing the value over time of parameter ``count`` for process ``C``.
