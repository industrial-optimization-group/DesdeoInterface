Each node has a some type (uint8_t) which we call 'NodeType'. The type is send to the master
and from there to Serial in the configuration state. This allows identification of each node: We can determine what kind of components a node has and the count of these components.

A nodes type can be changed at any point with the 'F' Serial command which takes 'isMaster' and 'nodeType' as arguments, see [PacketIds](../PacketIds) for further details on different commands. 

| NodeType      | type                |
| ------------- | -------------       |
| 0             | Empty               |
| 1             | Master              |
| 2             | One potentiometer   |
| 3             | One rotary encoder  |
| 4             | One button          |
| 5             | Screen              |
| 6             | Two potentiometers  |

This is needed because we are using the same code for each node so to avoid sending false values etc we tell each node
which pins should be used. 
Because of this each component must always be in the same pin(s) meaning that the component pins are predetermined: see [Schematics](../Schematics) for further details. 

A drawback to this (at least for the moment) is that adding new types means that we have to add them to the datatypes.h file AND to the other side of the Serial port.
One way to tackle this would be to have some file for each type: This might be more complicated than what is sounds...