# Packet ids for serial communication
Communication between master and pc through serial port. 
Here S and M stand for Serial port and Master respectively.
All lines written to serial should be of form "ID dataString CRC".

| ID            | Explanation         | Direction  | dataString         |
| ------------- | -------------       | -----      | -----              |
| S             | Start configuration | S -> M     | NONE               |
| Q             | Quit                | S -> M     | NONE               |
| N             | Node info           | M -> S     | id:p:r:b           |
| C             | Node connected      | M -> S     | NONE               |
| D             | Node disconnected   | M -> S     | id:dir             |
| V             | Component value     | M -> S     | nId:type:cId:value |

# Packet ids for PJON communication
Communication between master and the slaves with the PJON software bitbang protocol.
Here S and M stand for Slave and Master respectively and CS for configuration state
All packets, except data packet which contains component values, should be byte arrays where
the first index is the id of the packet.

| ID            | Explanation         | Direction  | Packet             |
| ------------- | -------------       | -----      | -----              |
| N             | Node info           | S -> M     | [id, p, r, b]      |
| D             | Node disconnected   | S -> M     | [id, dir]          |
| C             | Node connected      | S -> M     | NONE               |
| NONE          | Component value     | S -> M     | struct*            |
| S             | Start configuration | M -> S     | NONE               |
| R             | Reset self          | M -> S     | NONE               |
| I             | Dynamic id from M   | M -> S     | [id]               |
| N             | Instructions for CS | M -> S     | [dir]              |
| O             | CS completed        | M -> S     | NONE               |
| D             | Direction to check  | M -> S     | [dir]              |
* Component values are send in a self defined struct datatype which consists of node id (uint8_t), component value (uint16_t), component id (uint8_t) and the component type (char).