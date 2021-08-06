/*
Datatypes.h - Library for all the datatypes used in the physical interface
*/

#ifndef Datatypes_h
#define Datatypes_h

#include "Arduino.h"

/**
 * A type to hold counts for each type of component
 * Used with NodeType structure. 
 */
typedef struct {
    uint8_t potCount = 0;
    uint8_t rotCount = 0;
    uint8_t butCount = 0;
} ComponentCounts; 

/**
 * This type contains information of bounds for a node and its component
 */
typedef struct {
    char componentType; // The components type
    uint8_t componentId; // The components id
    double minValue; // The components min value bound
    double maxValue; // The components max value bound
    double stepSize; // The components step size. Potentiometers will ignore.
} BoundsData;

/**
 * This type contains information of node component value
 */
typedef struct
{
  uint8_t nodeId; // Sending node id
  double value; // The components value
  uint8_t id; // The components id
  char type; // The components type
} Data;

/**
 * Structure used in the initial 'understanding layout' phase.
 */
typedef struct
{
  uint8_t nodeId; // A node id
  uint8_t dir = 0; // The next direction of that node
} StackPair;

/**
 * Contains all the different types a node can be.
 * Make sure to add these to the interface side.
 * 
 * To upload a type to a node, one can send 
 * an command through serial to the node of type:
 * F {nodetype}
 * In case an invalid nodetype is assigned the node will assume to be empty
 */
enum NodeType {
    empty = 0, // Has nothing
    button = 1, // Has only a button connected
    potentiometer = 2, // Has only a potentiometer connected
    rotaryEncoder = 3, // Has only a rotary encoder connected
};

/**
 * Definitions for all the different commands that can be send via Serial or PJON
 * 
 * Each packet should start with one of these
 * i.e: 'N 1:2:23 crc' Would mean that the packet contains node info
 * that is id:type:pos. Packets send in Serial also end in a crc8 checksum
 */
const char configure = 'F'; // Configure a node, see above
const char nodeInfo = 'N'; // Node info that is written to serial in the configuration state. id:type:pos
const char nodeConnected = 'C'; // A node connected after the configuration state
const char nodeDisconnected = 'D'; // A node disconencted after the configuration state
const char componentValue = 'V'; // Component values being sent
const char start = 'S'; // Start the configuration
const char quit = 'Q'; // Exit the configuration and initialize self.
const char toInitial = 'E'; // Not used?
const char idPacket = 'I'; // An id packet the master sends to a node in the configuration state
const char dirInstruction = 'N'; // An direction instruction packet the master sends to a node in the configuration state
const char csCompleted = 'O'; // Stating that the configuration state is completed. May proceed to main loop
const char dirToCheck = 'D'; // Which directions should be checked after the configuration state
// Only send from Master to Node, nodeDisconnected (also 'D') is only send from slave to master and master to serial
// And that's why it can be duplicated.

const char bounds = 'B'; // A bounds struct packet


/**
 * Different state colors
 */

const uint8_t initialStateColor[3] = {255, 0, 0}; // RED
const uint8_t configuredColor[3] = {0, 255, 0}; // GREEN
const uint8_t sendingDataColor[3] = {0, 255, 0}; // BLUE
const uint8_t inConfigurationStateColor[3] = {255, 0, 255}; // MAGENTA

/*
 * Function: getCounts 
 * --------------------
 * Gets the component counts based on a nodetype
 * 
 * n: A nodetype
 *
 * returns: A ComponentCounts type which contains the count of each component 
 *  that corresponding notetype has. Empty node if an invalid nodetype.
 */
ComponentCounts getCounts(NodeType n)
{
   ComponentCounts counts;
   switch (n)
   {
    case button:
        counts.butCount = 1;
        break;
    case potentiometer:
        counts.potCount = 1;
        break;
    case rotaryEncoder:
        counts.rotCount = 1;
        break;
    default: 
        break;
    }
   return counts;
}

#endif
