# Physical interface for desdeo framework

Currently the nodes communicate with each other using the [PJON software bitbang](https://www.pjon.org/SoftwareBitBang.php) protocol. 
Each node has it's own dynamically assigned unique id which the master is aware of. This allows the master to communicate with a specific node. The master also knows the location and connected components of each node. This is achieved by a [configuration checker](TODO) which runs when the master reads the byte 'R' from the serial. 

In configuration state (CS) each node is assigned an id by the master and each node sends it basic information to master (atm: How many components of each type it has).
After the CS, nodes repeatedly (every 1-2 seconds, to avoid interferences) send an "ALIVE" signal to the master so that it can check if a node gets disconnect. If such thing should happen, we'll wait until the node is connected back and run the configuration state again. if the disconnected node has a new position, it should be possible, at least if only one node is disconnected, to maintain the ids of each node, even the disconnected one with the new position. In addition to sending the "ALIVE" signal, the nodes (obviously) send the values of the components to the master (sending the values counts as an "ALIVE" signal). These values are only sent when a value of a component changes and only that component value is sent along with the node id, component id and type (i.e. [421, 2, 'P', 1] which translates back to [value, node id, component type, component id]). After the master receives such data from a node it will read it and update a [JSON](https://arduinojson.org/) (check [Masters JSON](#2-masters-json) for more details) with the values received and then writes the whole JSON to serial, which can the be used wherever needed. 

Each node, including the master, has (any) 4 digital pins reserver for each direction (TOP, RIGHT, BOTTOM, LEFT) which are used by the configuration checker. Shortly we go node by node checking each direction, for more details check [configuration checker](TODO). In addition to these direction pins each node also needs 2 digital pins (ATmega32u4 supports pins 2,4,8,12) for communication. Currently 1 analog pin is used for generating a random seed but this is not necessary. The master communicates through serial so pins tx/rx should be left empty.

At the moment each node supports as many rotary encoder, potentiometer and button components as one can connect (only limited by pin count). It should be quite easy to add new components such as lcd screens, haptic sensors and so on. 

## Masters JSON 
We might go for some other way of writing data to serial instead of JSON. We use the [ArduinoJson library](https://arduinojson.org/).
The json looks something like this
{
    "master": {"accept": 0, "decline: 1, "rotary":231},
    "1": {"P": {"0": 1021, "1":22}, "B": {"0":1}},
    "2" {"R"}: {"0": 12}, 
}.
Here the master has it's base components two buttons and a rotary encoder, every other node can have anything else. In this case we two nodes where the first node has 2 potentiometers and one buttons and the second node only has a rotary encoder. 

## Todo
* Testing
    * Rotary encoders
    * multiple components
    * multiple nodes
* Disconnected node
    * Master should detect this quicker
    * Actions when disconnection happens
* Dynamic components, currently one must assign them in code

