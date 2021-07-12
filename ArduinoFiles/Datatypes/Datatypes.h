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
} bounds;

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
    display = 5, dualPot = 6
};

enum Command {
    configure = 'F';
    nodeInfo = 'N';
    nodeConnected = 'C'; 
    nodeDisconnected = 'D';
    componentValue = 'V';
    start = 'S';
    quit = 'Q';
    reset = 'E';
    idPacket = 'I';
    dirInstruction = 'N';
    csCompleted = 'O';
    dirToCheck = 'D';
};

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
    default: // empty
        break;
    }
   return node;
}

#endif
