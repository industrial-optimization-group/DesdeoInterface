# Direction Pins
Initially is responsible of assigning dynamic ids to each node in the configuration and finding their location. 
After this each pin is used to check for disconnecting nodes.

### Physical setup
On each side of the node we have a 5-pin connector with a pin dedicated only for a direction.
The direction pin is unique for each connector. This pin is used to: 
* determine the layout of the configuration
* assign dynamic ids to all nodes
* Check if neighboring nodes are still connected
Digital pins 7, 15, 14, 16 (top, right, bottom, left) are used for the direction pins.

![A node](.\connections.svg)

### Calculating positions
For calculating positions of the nodes, the method uses a stack of pairs which is implemented with a struct array called stack. The struct stackPair consist of a node id and the next direction it should check, both are bytes. We keep track of the index of the node highest in the stack in its own variable called stackTop. Initially the stackTop is equal to zero pointing at the master with the masters next direction being 0 (TOP). Every new node entering the stack start with the direction 0. A nodes position is calculated when it is found by going through the stack and checking the directions we have taken. In example if the current stack is{0: LEFT, 3: TOP, 4: TOP, 6: BOTTOM} and a new node is found it must be in position LEFT TOP TOP BOTTOM or 3002. The stack is then updated. This direction is send to serial and it is NOT saved anywhere on the master side.

### Finding the layout

The configuration method is used to find the location of each node. In addition,it is used to assign each node a unique id. The method is based on setting the direction pins high one by one for each node and waiting for responses from the receiving side. This means that the configuration method could be split into two parts, one for the master and other for the nodes. The master is mainly responsible for doing the configuration and all variables mentioned are on the master side if not mentioned otherwise. The stack mentioned in previous section is the building block of this method. The stack is updated on every call and is responsible of keeping track of the current position. The stack is a local variable which is passed to the method every call instead of being a global variable. This is to save some memory. To assign a unique id to each node we use a running integer starting from 1 which can go up to 253. We call this nextId. Id 0 is reserved for broadcasting and 254 for the master. This means that the configuration can have up to 253 nodes excluding the master. 

In addition,we use some flags to avoid certain situations: Both the master and the nodes have an isConfigurationDone flag, which is used to stop executing the receiver function and other functions before configuration is done. The nodes have a waitingForSignal flag which is only true when in the configuration state and the node has not received a signal. When a node receives a signal in the configuration state for the first time ‘waitingForSignal’ flag will be set back to false to avoid loops in the configuration. The isCurrentReceiver flag is for assigning unique ids to the node as the master is initially unable to send packets to a specific node. With this flag the master can broadcast a packet and only the node with this flag up will be able to know that the packet is meant for it.

The configuration starts when the master reads the byte ‘S’ from the serial port. The master then sends the start signal to each node in the configuration. The nodes react by setting the ‘waitingForSignal’ flag to true and all their direction pins to input. Now we proceed to the actual configuration method: First we check the next direction of the stackTop, we call this dir. Next, we check whether the master is on top of the stack or not by checking the stackTop integer, here 0 would mean that master is at top. The next steps are slightly different for the master and the nodes but in both cases, we check whether dir is valid, meaning that is either 0, 1, 2 or 3:
* Master: 
    * dir is valid: Set the pin in dir to high and all others to low
    * otherwise: Master has checked all its directions->The configuration is done, STOP by setting the isConfigurationDone flag to true and broadcast a packet informing the nodes that the configuration is done. Also send a configurationDone message to Serial.
* Node: 
    * dir is valid: Master sends a packet with dir to this node. The node will then set pin in dir to high and all others to low
    * otherwise: node has checked all its direction -> We can remove this node from the stack and move to the one below it by decrementing stackTop by one and RETURNing -> The configuration method calls itself so the execution will be continued from +

Now the master or a node has set their pin in dir to high.
If a node, with waitingForSignal flag set to true notices that a pin is set to high, it reacts by setting the waitingForSignal flag to false, the isCurrentReceiver flag to true and sends a [nodeType](doc\NodeTypes) packet to the master.It is important to notice that no packet is send if no node received the signal or is not waiting for the signal. This is because next the master checks whether it has received a packet (the master only waits 0.05 seconds for each packet):If a packet is received, then a new node has been found: We calculate the position of this new node and add it to the stack. Then the master broadcasts an id and increments both nextId and stackTop by one. Every node will receive this id packet but only the node with the isCurrentReceiver flag set to true will set the received id as its own id. After receiving the id,the node sets the isCurrentReceiver flag back to false. We call the configuration method with the updated stack.

+Finally, we increase dir by one (and update the stack) and call the configuration method. 

### Disconnecting nodes
In the CS a node gets notified by the master if a node was found from a direction it set high. This means that each node knows which 
nodes it has found. These directions are saved in a boolean array.

After the CS a node sets each direction pin:
* Input if a node was found from that direction
* Output LOW otherwise.
Now each node is 'looked' after by some other node (possibly master). If a node gets disconnected then a node which found the node will notice that the direction pin is HIGH instead of LOW. Then it will send a packet to the master which has its own id and the direction of the disconnected node. 



