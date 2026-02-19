#ifndef NODE_MAP_H
#define NODE_MAP_H

#include <stdint.h>
#include <open62541/server.h>
#include "uthash.h"

typedef struct {
    uint16_t id;         
    UA_NodeId nodeId;
    UT_hash_handle hh;
} NodeIdMapEntry;

/* Function Prototypes */
void clearMap(NodeIdMapEntry **nodeMap);
void addNodeIdToMap(uint16_t id, UA_NodeId *nodeId, NodeIdMapEntry ** nodeMap);
UA_NodeId* findNodeInMap(uint16_t id ,NodeIdMapEntry * nodeMap);

#endif