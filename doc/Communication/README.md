## Packet sending
A packet is sent with the PJON SoftwareBitBang protocol [https://www.pjon.org/SoftwareBitBang.php].
To receive a packet a node must call the bus.receive function. 
Packets can be blocking, meaning they are sent until received or some other condition. 
We can check the status of each sent packet: was it received etc... 
Each packet is accompanied with a packet id, see below.
Whenever a packet is received we call the receiver_function, read the packet id and do actions
depending on the packet id. 

## Packet ids
Makes packets identifiable which makes communication between master - pc and master - node simpler. 
Each packet has an identifier expect those which only contain a struct datatype, see bounds and component value. 
There's only one unidentified packet per communication direction and therefore it is fine. adding one more would break this.
* A fix to this would be good: Add id to struct packets.

The nodes read the packet ids from the datatypes.h headerfile which is located in [desdeo_interface/ArduinoFiles/Datatypes](desdeo_interface/ArduinoFiles/Datatypes).
The ids are checked in the receiver function or the masters main loop if data is coming from serial.

### Packet ids for PJON communication
Communication between the master and the nodes is handled with the PJON software bitbang protocol.
Here N and M stand for node and master respectively and CS for configuration state.
All packets, except data packet which contains component values, should be byte arrays where
the first index is the id of the packet.

| pID            | Explanation         | Direction  | Packet starting from second index as first index is pID        |
| ------------- | -------------       | -----      | -----          |
| N             | Node info           | N -> M     | [id, type]  |
| D             | Node disconnected   | N -> M     | [id, dir]      |
| C             | Node connected      | N -> M     | NONE           |
| NONE          | Component value     | N -> M     | struct*        |
| S             | Start configuration | M -> N     | NONE         |
| E             | Reset self          | M -> N     | NONE          |
| I             | Dynamic id from M   | M -> N     | [id]           |
| N             | Instructions for CS | M -> N     | [dir]          |
| O             | Configuration done  | M -> N     | NONE           |
| D             | Direction to check  | M -> N     | [dir]          |
| B             | Bounds for node     | M -> N     | struct**       |

*Component values are send in a self defined struct datatype which consists of node id (uint8_t), component value (uint16_t), component id (uint8_t) and the component type (char).
**Bounds are send as struct with values component type, component id, min value, max value, step size. Step size will be ignored for components of type potentiometer 'P'.

### Packet ids for serial communication
Communication between master and pc through serial port. 
Here S and M stand for Serial port and the master respectively.
All lines written to serial should be of form "ID dataString CRC", expect those which only contain the packet id, i.e. configuration done.
(A node is assumed to be the master if they receive data from serial)

| ID            | Explanation         | Direction  | dataString         |
| ------------- | -------------       | -----      | -----              |
| F*            | Configure node      | M -> S     | type               |
| S             | Start configuration | S -> M     | NONE               |
| O             | Configuration done  | S -> M     | NONE               |
| Q             | Quit                | S -> M     | NONE               |
| E             | Reset               | M -> S     | NONE               |
| N             | Node info           | M -> S     | id:type:pos        |
| C             | Node connected      | M -> S     | NONE               |
| D             | Node disconnected   | M -> S     | id:dir             |
| V             | Component value     | M -> S     | nId:type:cId:value |
| B             | Bounds for node     | M -> S     | nId:compType:compId:minValue:maxValue:stepSize | 

*Not currently implemented but could be beneficial

### CRC checking

A basic 8-bit Cyclic redundancy check (CRC-8) is used to validate the Serial data. The CRC key used for the checksums is 7.
You can find the CRC implementation [here](/desdeo_interface/ArduinoFiles/CRC8). Using a lookupTable is advised but as it requires a decent amount of memory we don't, at least for now, use it (It's implemented but commented out).
Using it on the PC side is recommended as it fast up the crc process.

Crc validator is not yet implemented on the arduino side. Meaning that crc is used only when sending data from master to Serial. This should be a TODO.

The implementation returns same results as this online crc calculator (algorithm = CRC-8): [https://crccalc.com/](https://crccalc.com/)

Python crc validator with a lookup table: 
```python

crc_lookup_table = create_lookup_table(crc_key)

def create_lookup_table(self, key: np.uint8):
table = np.zeros(256)
for i in range(256):
    cur: np.uint8 = i
    for _ in range(8):
        if (np.bitwise_and(cur, 0x80)) != 0:
            cur = np.left_shift(cur, 1) % 256
            cur = np.bitwise_xor(cur, key)
        else:
            cur = np.left_shift(cur, 1) % 256
    table[i] = cur
return table

def crc_check(self, data: str, crc: np.uint8) -> bool:
remainder = 0
data_arr = [ord(s) for s in data] + [crc]
for d in data_arr:
    remainder = crc_lookup_table[d ^ remainder]
return remainder == 0

```