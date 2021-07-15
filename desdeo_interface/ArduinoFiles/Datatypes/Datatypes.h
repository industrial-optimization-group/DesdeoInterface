/*
Datatypes.h - Library for all the datatypes
*/

#ifndef Datatypes_h
#define Datatypes_h

#include "Arduino.h"


typedef struct {
    uint8_t potCount = 0;
    uint8_t rotCount = 0;
    uint8_t butCount = 0;
} node; 

typedef struct {
    char componentType;
    uint8_t componentId;
    double minValue;
    double maxValue;
    double stepSize;
} bounds_data;

typedef struct
{
  uint8_t nodeId;
  double value;
  uint8_t id;
  char type;
} data;

enum Node {
    empty = 0,
    master = 1,
    singlePot = 2, singleRot = 3, singleBut = 4,
    display = 5, dualRot = 6
};


const char configure = 'F';
const char nodeInfo = 'N';
const char nodeConnected = 'C'; 
const char nodeDisconnected = 'D';
const char componentValue = 'V';
const char start = 'S';
const char quit = 'Q';
const char reset = 'E';
const char idPacket = 'I';
const char dirInstruction = 'N';
const char csCompleted = 'O';
const char dirToCheck = 'D';
const char bounds = 'B';

// Or array of nodes?
node getNode(Node n)
{
   node node;
   switch (n)
   {
    case master:
        node.rotCount = 1;
        node.butCount = 2;
        break;
    case singlePot:
        node.potCount = 1;
        break;
    case singleRot:
        node.rotCount = 1;
        break;
    case singleBut:
        node.butCount = 1;
    case dualRot:
        node.rotCount = 2;
    default: // empty
        break;
    }
   return node;
}

#endif
