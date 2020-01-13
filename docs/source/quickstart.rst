.. _quickstart:

Quickstart
===============================

To get something running and see how to use bcs, let's run one of the tests. Models should be written and saved in a separate file.  These files are passed to the bcs binary on the command line.  Navigate to the bcs directory and run: ::

   bin/bcs -s 50 -t 1 -o firstSimulation tests/shouldPass/beacons-basic.bc

Here, we've specified the path to a model file (tests/shouldPass/beacons-basic.bc), the number of simulations we want to run (50), and the number of threads to use (1).  There should now be a file called firstSimulation.simulation.bcs in your current directory.  If we open this file, the first few lines should look like this: ::

   >=======
   0.015743        msg     proc2   j       3
   0.151068        action2 proc2   j       3
   0.160805        msg     proc1   j       3
   1.14175e+06     longAction      proc1   j       3
   >=======
   0.202562        msg     proc2   j       3
   0.231809        action2 proc2   j       3
   0.315113        msg     proc1   j       3
   303730  longAction      proc1   j       3
   >=======
   0.0960461       msg     proc2   j       3
   0.11236 msg     proc1   j       3
   0.229927        action2 proc2   j       3
   569664  longAction      proc1   j       3

bcs output files all have the same format, where the start of each new simulation is marked with ">=======" and each row contains information about an action that a process performed during the simulation.  From left to right, the tab-delimited columns specify the following information:​ the time when an action was done, the action name (if it was a non-messaging action) or channel name (if it was a handshake or beacon action), the process name that performed the action, the value of that process's parameters (if it has any) when the action was performed.
