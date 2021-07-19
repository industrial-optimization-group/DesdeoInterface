# Packet ids
Make packets identifiable which makes communication between master - pc and master - node simpler. 
Each packet has an identifier expect those which only contain a struct datatype, see bounds and component value. 
There's only one unidentified packet per communication direction and therefore it is fine. adding one more would break this.
* A fix to this would be good

The nodes read the packet ids from the datatypes.h headerfile which is located in [desdeo_interface/ArduinoFiles/Datatypes](desdeo_interface/ArduinoFiles/Datatypes).
The ids are checked in the receiver function or the main loop if data is coming from serial.

# Packet ids for serial communication
Communication between master and pc through serial port. 
Here S and M stand for Serial port and Master respectively.
All lines written to serial should be of form "ID dataString CRC".

| ID            | Explanation         | Direction  | dataString         |
| ------------- | -------------       | -----      | -----              |
| F*            | Configure node      | M -> S     | isMaster:type      |
| S             | Start configuration | S -> M     | NONE               |
| O             | Configuration done  | S -> M     | NONE               |
| Q             | Quit                | S -> M     | NONE               |
| R             | Reset               | M -> S     | NONE               |
| N             | Node info           | M -> S     | id:p:r:b           |
| C             | Node connected      | M -> S     | NONE               |
| D             | Node disconnected   | M -> S     | id:dir             |
| V             | Component value     | M -> S     | nId:type:cId:value |
| B             | Bounds for node     | M -> S     | nId:               |

*Any node can get the configuration command from serial not only the master!

# Packet ids for PJON communication
Communication between master and the slaves with the PJON software bitbang protocol.
Here S and M stand for Slave and Master respectively and CS for configuration state
All packets, except data packet which contains component values, should be byte arrays where
the first index is the id of the packet.

| ID            | Explanation         | Direction  | Packet         |
| ------------- | -------------       | -----      | -----          |
| N             | Node info           | S -> M     | [id, p, r, b]  |
| D             | Node disconnected   | S -> M     | [id, dir]      |
| C             | Node connected      | S -> M     | NONE           |
| NONE          | Component value     | S -> M     | struct*        |
| S             | Start configuration | M -> S     | NONE           |
| R             | Reset self          | M -> S     | NONE           |
| I             | Dynamic id from M   | M -> S     | [id]           |
| N             | Instructions for CS | M -> S     | [dir]          |
| O             | Configuration done  | M -> S     | NONE           |
| D             | Direction to check  | M -> S     | [dir]          |
| B             | Bounds for node     | M -> S     | struct**       |

*Component values are send in a self defined struct datatype which consists of node id (uint8_t), component value (uint16_t), component id (uint8_t) and the component type (char).
**Bounds are send as struct with values component type, component id, min value, max value, step size. Step size will be ignored for components of type potentiometer 'P'.