We use Serial communication between the master and pc.
Serial is also used to set a node type for any node. 

### Packet ids for serial communication
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