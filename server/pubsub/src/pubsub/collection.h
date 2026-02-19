#ifndef COLLECTION_H
#define COLLECTION_H

#include <stdint.h>
#include <open62541/server.h>

typedef struct {
    char * mqttClientId;
    char * topic;
    bool useJson;
    bool useMqtt;     
    int metaUpdateTime;
    char * metaQueueName;
} ServerConfig;
/* Grouped NodeIds for OPC UA objects */
typedef struct {
    UA_NodeId subConId;
    UA_NodeId pubConId;
    UA_NodeId subNodeId;
    UA_NodeId pubNodeId;
    UA_NodeId grpNodeId;
    UA_NodeId dtsNodeId;
    UA_NodeId readGroupId;
    UA_NodeId readerId;
    UA_NodeId fieldNodeId; 
    UA_NodeId publishedDataSetId;
    UA_NodeId writerGroupId;
} NodeCollection;

/* Protocol-specific identifiers for PubSub */
typedef struct {
    uint16_t publisherId;
    uint16_t groupId;
    uint16_t writerId;
    uint16_t nsIndex; 
} PubSubIdentifiers;

/* Main container for all related IDs */
typedef struct {
    NodeCollection nodes;
    PubSubIdentifiers identifiers;
    ServerConfig config;
} ServerContext;

/* Function Signatures */
void ServerContextInit(ServerContext *ctx);
void ServerContextClear(ServerContext *ctx);
char* ServerContextToString(const ServerContext *ctx);



typedef struct {
    UA_NodeId conId;
    UA_NodeId writerGroupId;
    UA_NodeId publishedDataSetId;
    UA_NodeId fieldNodeId;
    UA_UInt64 callbackId;
    uint32_t  publisherId;
    uint16_t  groupId;
    uint16_t  writerId;
    uint16_t  namespace;
} Pub_IdCollection;
void Pub_IdCollection_clear(Pub_IdCollection * ids);
void Pub_IdCollection_init(Pub_IdCollection *ids);
char *Pub_IdCollection_toString(const Pub_IdCollection *ids);


typedef struct {
    UA_NodeId conId;
    UA_NodeId subNodeId;
    UA_NodeId pubNodeId;
    UA_NodeId grpNodeId;
    UA_NodeId dtsNodeId;
    UA_NodeId readGroupId;
    UA_NodeId readerId;
    uint32_t  publisherId;
    uint16_t  groupId;
    uint16_t  writerId;
    uint16_t  namespace;
} Sub_IdCollection;

void Sub_IdCollection_clear(Sub_IdCollection * ids);
void Sub_IdCollection_init(Sub_IdCollection *ids);
char *Sub_IdCollection_toString(const Sub_IdCollection *ids);
#endif 