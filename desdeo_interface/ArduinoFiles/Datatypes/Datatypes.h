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
} ComponentCounts; 

typedef struct {
    char componentType;
    uint8_t componentId;
    double minValue;
    double maxValue;
    double stepSize;
} BoundsData;

// This is the data structure that is send to the master
typedef struct
{
  uint8_t nodeId;
  double value;
  uint8_t id;
  char type;
} Data;

typedef struct
{
  uint8_t nodeId;
  uint8_t dir = 0;
} StackPair;

enum NodeType {
    empty = 0,
    master = 1,
    singlePot = 2, singleRot = 3, singleBut = 4,
    display = 5, dualRot = 6
};

struct MyEeprom
{
  bool configured;
  bool isMaster;
  NodeType type;
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

// Or array of nodes? this might actually be more hassle free in the future
// Say you want to add a new component x, now with array you would have to add that x to every type
// -> {0,0,0} -> {0,0,0,0} and so on. 
ComponentCounts getCounts(NodeType n)
{
   ComponentCounts counts;
   switch (n)
   {
    case master:
        counts.rotCount = 1;
        counts.butCount = 2;
        break;
    case singlePot:
        counts.potCount = 1;
        break;
    case singleRot:
        counts.rotCount = 1;
        break;
    case singleBut:
        counts.butCount = 1;
    case dualRot:
        counts.rotCount = 2;
    default: 
        break;
    }
   return counts;
}

#endif
