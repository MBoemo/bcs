.. _system_line:

The System Line
===============================

The initial state of a Beacon Calculus system is specified using a system line.  Only one system line is allowed in the model file, and it must be the last line in any bcs model.  The system line specifies the processes present in the system at time zero; specifically, how many of each process we have and their parameter values.  For example, a complete piece of bcs source code is: ::

   driveRate = 0.1; //static variable definition

   Car[i] = [i < 10] -> {drive,0.1}.Car[i+1]; //process definition

   Car[0]; //system line

The first line specifies a variable definition.  The second line specifies the process definition for a car as we've done previously.  The third and final line is the system line which specifies that at time zero, there is one copy of a car process in the system and the value of its parameter is ``i=0``.  Suppose instead we wanted 50 cars, each of which start at ``i=0``.  Then we can write: ::

   driveRate = 0.1;
   initialCars = 50;

   Car[i] = [i < 10] -> {drive,0.1}.Car[i+1];

   initialCars*Car[0];

If we wanted 50 cars to start at ``i=0`` and 20 cars to start at ``i=5``, we can write: ::

   driveRate = 0.1;
   initialCars = 50;

   Car[i] = [i < 10] -> {drive,0.1}.Car[i+1];

   initialCars*Car[0] || 20*Car[5];
