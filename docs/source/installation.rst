.. _installation:

Installation
===============================

bcs is hosted on Github.  To clone bcs into your current directory, run​ ::

   git clone https://github.com/MBoemo/bcs.git​
   git checkout v1.0

Navigate to the bcs directory and run ::

   make

This will compile bcs and put a binary into the bcs/bin directory.  bcs was written in C++11 and uses OpenMP for parallel computation.  These are both standard on most systems and there are no other thirdparty dependencies.  bcs was tested to compile on both Linux systems and OSX.
