#include <stdint.h>
#include <open62541/server.h>
#include "uthash.h"
#include "nodeIdMap.h"


void clearMap(NodeIdMapEntry **nodeMap) {
    if (nodeMap == NULL || *nodeMap == NULL) {
        return; // Already empty, nothing to do
    }

    NodeIdMapEntry *current_entry, *tmp;
    HASH_ITER(hh, *nodeMap, current_entry, tmp) {
        HASH_DEL(*nodeMap, current_entry);  
        
        // Only clear if the nodeId is not null/empty
        if (current_entry != NULL) {
            UA_NodeId_clear(&current_entry->nodeId);
            free(current_entry);
        }
    }
    *nodeMap = NULL; // Crucial: Reset the pointer to NULL after clearing
}

void addNodeIdToMap(uint16_t id, UA_NodeId *nodeId, NodeIdMapEntry ** nodeMap) {
    NodeIdMapEntry *entry;
    
    // Check if ID already exists
    HASH_FIND_INT(*nodeMap, &id, entry);
    
    if (entry == NULL) {
        entry = (NodeIdMapEntry*)malloc(sizeof(NodeIdMapEntry));
        entry->id = id;
        // Deep copy the NodeId so we don't have dangling pointers
        UA_NodeId_copy(nodeId, &entry->nodeId);
        HASH_ADD_INT(*nodeMap, id, entry);
    }
}

UA_NodeId* findNodeInMap(uint16_t id ,NodeIdMapEntry * nodeMap) {
    NodeIdMapEntry *entry;
    HASH_FIND_INT(nodeMap, &id, entry);
    return (entry) ? &entry->nodeId : NULL;
}