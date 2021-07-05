# Physical interface for desdeo framework

Currently the nodes communicate with each other using the [PJON software bitbang](https://www.pjon.org/SoftwareBitBang.php) protocol. 
Each node has it's own dynamically assigned unique id which the master is aware of. This allows the master to communicate with a specific node. The master also knows the location and connected components of each node. This is achieved by a [configuration checker](doc\ConfigurationChecker) which runs when the master reads the byte 'R' from the serial. 

In configuration state (CS) each node is assigned an id by the master and each node sends it basic information to master (atm: How many components of each type it has).
After the CS. The nodes send the values of the components to the master. These values are only sent when a value of a component changes and only that component value is sent along with the node id, component id and type (i.e. [421, 2, 'P', 1] which translates back to [value, node id, component type, component id]). After the master receives such data from a node it will read it and write the values to Serial port which can the be used whenever needed.

If a node gets disconnect after the CS a node next to it will notice this immediately and send a packet to the master telling a node from side x of me has disconnected, the master can then pass this information to the serial and the id of the disconnected node can be easily verified if the position of each slave is saved. 

Whenever a node connects it send a message to the master indicating that is has connected. If the configuration state is not done the master will ignore this message otherwise the master receives the message and runs the configuration again since a new node has connected.

Each node, including the master, has (any) 4 digital pins reserver for each direction (TOP, RIGHT, BOTTOM, LEFT) which are used by the configuration checker. Shortly we go node by node checking each direction, for more details check [configuration checker](doc\ConfigurationChecker). In addition to these direction pins each node also needs 2 digital pins (ATmega32u4 supports pins 2,4,8,12) for communication, in\out. Currently one analog pin is used for generating a random seed but this is not necessary. The master communicates through serial so pins tx/rx should be left empty.

At the moment each node supports as many rotary encoder, potentiometer and button components as one can connect (only limited by pin count). It should be quite easy to add new components such as lcd screens, haptic sensors and so on. 

## Todo
* Testing
    * Rotary encoders
    * multiple components
    * multiple nodes
    * Unexpected interrupts, i.e user ctrl-c:s from program
* Dynamic components, currently one must assign each connected component manually in code

## Issues

