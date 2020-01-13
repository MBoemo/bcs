.. _communication:

Communication
===============================

Handshakes
----------

Processes need to be able to interact with one another.  In doing so, they can change their actions in response to other processes.  In the Beacon Calculus, two processes can communicate synchronously (at the same time) via handshake actions:

* A handshake send action ``{@chan![i], rs}`` sends value ``i`` on channel chan at rate ``rs``.
* A handshake receive action ``{@chan?[S](x), rr}`` receives one of a set of values ``S`` on channel chan at rate ``rr`` and binds the result to ``x``.

If the handshake happens, the handshake receive and handshake send actions happen together at rate ``rs*rr``. 

Let's illustrate with an example.  A surveyor has been tasked to count the number of cars that travel down a road over time.  They surveyor is located at ``i=5``, and performs a handshake with each car as it goes by. ::

   driveRate = 0.1;
   initialCars = 50;
   fast = 10000;

   Car[i] = [i < 10 & i!= 5] -> {drive,driveRate}.Car[i+1] 
          + [i==5] -> {@count![0],fast}.{drive,driveRate}.Car[i+1];
   Surveyor[c] = {@count?[0],1}.Surveyor[c+1];

   initialCars*Car[0] || Surveyor[0];

This model beings with 50 cars at ``i=0`` and a surveyor that has counted 0 cars.  If a car isn't at ``i=5`` then it steps as normal.  If the car is at ``i=5`` then it handshakes with the surveyor at a fast rate using channel count.  Both the actions ``{@count![0],fast}`` and ``{@count![0],fast}`` happen simultaneously at rate ``fast*1 = fast``.  Once both a car and the surveyor perform the handshake, the surveyor increases their count by one by incrementing parameter ``c`` and recursing. The car carries on driving as normal.

In the above example, the handshake could receive a single value (5) on channel count.  However, as mentioned above, handshakes can accept a set of values on a particular channel.  Suppose the surveyor wants to keep a separate count of cars that are red. Consider the following model: ::

   driveRate = 0.1;
   nonRed= 25;
   red= 25;
   fast = 10000;

   Car[i,r] = [i < 10 & i!= 5] -> {drive,driveRate}.Car[i+1]
            + [i==5] -> {@count![r],fast}.{drive,driveRate}.Car[i+1];
   Surveyor[c,cr] = {@count?[0..1](x),1}.Surveyor[c+1,cr+x];

   nonRed*Car[0,0] || red*C[0,1] || Surveyor[0,0];

The Surveyor process has two parameters, ``c`` and ``cr``, which keeps track of the total count and red cars, respectively. Car has an additional parameter ``r`` which is either 1 (red) or 0 (not red).  The range operator ``..`` in ``{@count?[0..1](x),1}`` means the set of all consecutive integers between 0 and 1 (inclusive). In this case, this is the set {0,1}. So the handshake receive action accepts one of two possible values and binds what it receives to ``x`` for later use.  If the car it handshakes with is red, it receives value 1.  When the surveyor process recurses, ``x=1`` so it increments the red car count ``cr`` by one.  If the car was not red, then the value received would have been 0 so that ``x=0`` when the the process recurses.  Therefore, ``cr+x = cr+0 = cr`` so the red car count is not increased.

In addition the range (..) operator used above, bcs supports the following set operations for both beacons and handshakes:

* ``U``, set union, 
* ``I``, set intersection,
* ``\``, set subtraction.

For example, the following Beacon Calculus operations (left hand side) correspond to these sets (right hand side): ::

   -5..2 = {-5,-4,-3,-2,-1,0,1,2}
   1U8..10 = {1,8,9,10}
   1I8..10 = {}
   1\8..10 = {1}
   15..18\16 = {15,17,18}
   0..2U8..15I4..9 = {0,1,2,8,9}​

Beacons
-------

Handshakes provide the means for synchronous communication between processes, whereby two processes each perform handshake actions at the same time.  The Beacon Calculus also allows processes to communicate via beacons, which is asynchronous communication.  Any process can launch a beacon that transmits a value on a channel.  The beacon stays active until it is explicitly killed by a process (not necessarily the same process that launched it).

* A beacon launch, ``{chan![i],rs}`` launches a beacon that transmits value ``i`` on channel chan at rate ``rs``.
* A beacon kill, ``{chan#[i],rs}`` kills a beacon (if there is one) transmitting value ``i`` on channel chan at rate ``rs``.

Once a beacon is launched, processes can interact with active beacons in two ways.

* A beacon receive ``{chan?[S](x),rr}`` can only be performed if there is an active beacon on channel chan transmitting a value in set ``S``.  If there is such a beacon, a process can perform the beacon receive action and bind the value it receives to ``x`` for later use.
* A beacon check ``{~chan?[S],rr}`` is the inverse of a beacon receive. This action can only be performed if there is no active beacon on chan transmitting any value in ``S``.

While a beacon is active, it can be received any number of times by any number of processes. Once a beacon has been killed, it can no longer be received.  

Let's consider a simple example.  Suppose there is a traffic light at ``i=5`` that switches between red and green.  Cars can only pass through the intersection at ``i=5`` when the light is green.  Otherwise, they have to wait. ::

   driveRate = 0.1;
   change = 0.001;
   initialCars = 50;
   fast = 10000;

   Car[i] = [i < 10 & i!= 5] -> {drive,driveRate}.Car[i+1]
          + [i==5] -> {~red?[0],driveRate}.Car[i+1];
   TrafficLight[g] = [g==1] -> {green#[0],change}.{red![0],fast}.TrafficLight[0]
                   + [g==0] -> {red#[0],change}.{green![0],fast}.TrafficLight[1];

   initialCars*Car[0] || TrafficLight[0];

In the above model, there is a process ``TrafficLight`` with a parameter ``g``.  When ``g=1``, the traffic light is showing green.  When ``g=0``, the traffic light is showing red.  If the traffic light is showing green, it keeps a beacon active on channel ``green``.  When the traffic light switches, it kills the beacon on ``green`` and launches a new one on channel ``red``.  Switching from red back to green is similar.  In order for a car to move through the intersection at ``i=5``, it performs a beacon check to make sure the light is not red.  If the light is red, the car has to wait until the light turns green as it cannot perform the beacon check action while there is an active beacon on channel ``red``.  

In this example, we could have created a model where the traffic light handshakes with each car rather communicate via beacons.  However, this would have been slightly more cumbersome.  Beacons make it easy and concise to communicate a state change to a large number of other processes.


