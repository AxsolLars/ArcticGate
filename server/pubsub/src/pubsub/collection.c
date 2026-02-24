#include <stdint.h>   
#include <stddef.h>   
#include <stdio.h>
#include <string.h>
#include <open62541/server.h>

typedef struct {
    char * mqttClientId;
    char * topic;
    bool useJson;
    bool useMqtt; 
    int metaUpdateTime;
    char * metaQueueName;
} ServerConfig;
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

/* Renamed from IntCollection to PubSubIdentifiers */
typedef struct {
    uint16_t publisherId;
    uint16_t groupId;
    uint16_t writerId;
    uint16_t nsIndex;
} PubSubIdentifiers;

typedef struct {
    NodeCollection nodes;
    PubSubIdentifiers identifiers;
    ServerConfig config;
} ServerContext;

// --- Methods ---

void ServerContextInit(ServerContext *ctx) {
    if(!ctx) return;
    memset(ctx, 0, sizeof(ServerContext));
    
    // Initialize all NodeIds to NULL
    ctx->nodes.subConId           = UA_NODEID_NULL;
    ctx->nodes.pubConId           = UA_NODEID_NULL;
    ctx->nodes.subNodeId          = UA_NODEID_NULL;
    ctx->nodes.pubNodeId          = UA_NODEID_NULL;
    ctx->nodes.grpNodeId          = UA_NODEID_NULL;
    ctx->nodes.dtsNodeId          = UA_NODEID_NULL;
    ctx->nodes.readGroupId        = UA_NODEID_NULL;
    ctx->nodes.readerId           = UA_NODEID_NULL;
    ctx->nodes.fieldNodeId        = UA_NODEID_NULL;
    ctx->nodes.publishedDataSetId = UA_NODEID_NULL;
    ctx->nodes.writerGroupId      = UA_NODEID_NULL;

}

void ServerContextClear(ServerContext *ctx) {
    if(!ctx) return;
    ctx->nodes.pubNodeId          = UA_NODEID_NULL;
    ctx->nodes.grpNodeId          = UA_NODEID_NULL;
    ctx->nodes.dtsNodeId          = UA_NODEID_NULL;
    ctx->nodes.readGroupId        = UA_NODEID_NULL;
    ctx->nodes.readerId           = UA_NODEID_NULL;
    ctx->nodes.fieldNodeId        = UA_NODEID_NULL;
    ctx->nodes.publishedDataSetId = UA_NODEID_NULL;
    ctx->nodes.writerGroupId      = UA_NODEID_NULL;
    memset(&ctx->identifiers, 0, sizeof(PubSubIdentifiers));
}

char *ServerContextToString(const ServerContext *ctx) {
    if(!ctx) return NULL;

    /* 1. Extract strings for all 10 NodeIds */
    UA_String strings[10];
    for(int i = 0; i < 10; i++) strings[i] = UA_STRING_NULL;

    UA_NodeId_print(&ctx->nodes.subConId,           &strings[0]);
    UA_NodeId_print(&ctx->nodes.pubConId,           &strings[1]);
    UA_NodeId_print(&ctx->nodes.subNodeId,          &strings[2]);
    UA_NodeId_print(&ctx->nodes.pubNodeId,          &strings[3]);
    UA_NodeId_print(&ctx->nodes.grpNodeId,          &strings[4]);
    UA_NodeId_print(&ctx->nodes.readGroupId,        &strings[5]);
    UA_NodeId_print(&ctx->nodes.readerId,           &strings[6]);
    UA_NodeId_print(&ctx->nodes.fieldNodeId,        &strings[7]);
    UA_NodeId_print(&ctx->nodes.publishedDataSetId, &strings[8]);
    UA_NodeId_print(&ctx->nodes.writerGroupId,      &strings[9]);

    /* 2. Allocate a larger buffer for the full report */
    size_t bufSize = 2048; 
    char *buf = (char*)UA_malloc(bufSize);
    if(!buf) return NULL;

    /* 3. Build the massive string */
    snprintf(buf, bufSize,
             "ServerContext {\n"
             "  [Nodes]\n"
             "    subConId           = %.*s\n"
             "    pubConId           = %.*s\n"
             "    subNodeId          = %.*s\n"
             "    pubNodeId          = %.*s\n"
             "    grpNodeId          = %.*s\n"
             "    readGroupId        = %.*s\n"
             "    readerId           = %.*s\n"
             "    fieldNodeId        = %.*s\n"
             "    publishedDataSetId = %.*s\n"
             "    writerGroupId      = %.*s\n"
             "  [Identifiers]\n"
             "    publisherId        = %u\n"
             "    groupId            = %u\n"
             "    writerId           = %u\n"
             "    namespaceIndex     = %u\n"
             "  [Config (Environment)]\n"
             "    mqttClientId       = %s\n"
             "    topic              = %s\n"
             "    useJson            = %s\n"
             "    useMqtt            = %s\n"
             "    metaQueueName      = %s\n"
             "    metaUpdateTime     = %d\n"
             "}",
             /* NodeId Strings */
             (int)strings[0].length, (char*)strings[0].data,
             (int)strings[1].length, (char*)strings[1].data,
             (int)strings[2].length, (char*)strings[2].data,
             (int)strings[3].length, (char*)strings[3].data,
             (int)strings[4].length, (char*)strings[4].data,
             (int)strings[5].length, (char*)strings[5].data,
             (int)strings[6].length, (char*)strings[6].data,
             (int)strings[7].length, (char*)strings[7].data,
             (int)strings[8].length, (char*)strings[8].data,
             (int)strings[9].length, (char*)strings[9].data,
             /* Identifiers */
             ctx->identifiers.publisherId,
             ctx->identifiers.groupId,
             ctx->identifiers.writerId,
             ctx->identifiers.nsIndex,
             /* Config Strings (checking for NULL to avoid crashes) */
             ctx->config.mqttClientId ? ctx->config.mqttClientId : "NULL",
             ctx->config.topic ? ctx->config.topic : "NULL",
             ctx->config.useJson ? "true" : "false",
             ctx->config.useMqtt ? "true" : "false",
             ctx->config.metaQueueName ? ctx->config.metaQueueName : "NULL",
             ctx->config.metaUpdateTime);

    /* 4. Cleanup temporary UA_Strings (loop through all 10) */
    for(int i = 0; i < 10; i++) {
        UA_String_clear(&strings[i]);
    }

    return buf;
}





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
/* Clears everything except conId*/
void Pub_IdCollection_clear(Pub_IdCollection * ids){
    ids->writerGroupId = UA_NODEID_NULL;
    ids->publishedDataSetId = UA_NODEID_NULL;
    ids->fieldNodeId = UA_NODEID_NULL;
    ids->publisherId = 0;
    ids->groupId = 0;
    ids->writerId = 0;
    ids->namespace = 0;
}

void Pub_IdCollection_init(Pub_IdCollection *ids) {
    if(!ids) return;
    memset(ids, 0, sizeof(Pub_IdCollection));
    ids->conId       = UA_NODEID_NULL;
    ids->writerGroupId = UA_NODEID_NULL;
    ids->publishedDataSetId = UA_NODEID_NULL;
    ids->fieldNodeId = UA_NODEID_NULL;
}
/* for debugging only */
char *Pub_IdCollection_toString(const Pub_IdCollection *ids) {
    if(!ids) return NULL;

    UA_String conIdStr            = UA_STRING_NULL;
    UA_String writerGroupIdStr    = UA_STRING_NULL;
    UA_String publishedDataSetIdStr = UA_STRING_NULL;
    UA_String fieldNodeIdStr      = UA_STRING_NULL;

    if(!UA_NodeId_isNull(&ids->conId))
        UA_NodeId_print(&ids->conId, &conIdStr);
    if(!UA_NodeId_isNull(&ids->writerGroupId))
        UA_NodeId_print(&ids->writerGroupId, &writerGroupIdStr);
    if(!UA_NodeId_isNull(&ids->publishedDataSetId))
        UA_NodeId_print(&ids->publishedDataSetId, &publishedDataSetIdStr);
    if(!UA_NodeId_isNull(&ids->fieldNodeId))
        UA_NodeId_print(&ids->fieldNodeId, &fieldNodeIdStr);

    size_t bufSize = 512;
    char *buf = (char*)UA_malloc(bufSize);
    if(!buf) return NULL;

    snprintf(buf, bufSize,
         "Pub_IdCollection {\n"
         "  conId              = %.*s\n"
         "  writerGroupId      = %.*s\n"
         "  publishedDataSetId = %.*s\n"
         "  fieldNodeId        = %.*s\n"
         "  callbackId         = %llu\n"
         "  publisherId        = %u\n"
         "  groupId            = %u\n"
         "  writerId           = %u\n"
         "  namespace          = %u\n"
         "}",
         (int)conIdStr.length,            conIdStr.data            ? (const char*)conIdStr.data            : "",
         (int)writerGroupIdStr.length,    writerGroupIdStr.data    ? (const char*)writerGroupIdStr.data    : "",
         (int)publishedDataSetIdStr.length, publishedDataSetIdStr.data ? (const char*)publishedDataSetIdStr.data : "",
         (int)fieldNodeIdStr.length,      fieldNodeIdStr.data      ? (const char*)fieldNodeIdStr.data      : "",
         (unsigned long long)ids->callbackId,
         ids->publisherId,
         ids->groupId,
         ids->writerId,
         ids->namespace);

    UA_String_clear(&conIdStr);
    UA_String_clear(&writerGroupIdStr);
    UA_String_clear(&publishedDataSetIdStr);
    UA_String_clear(&fieldNodeIdStr);

    return buf;
}

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

void Sub_IdCollection_clear(Sub_IdCollection * ids){
    // clears everything except conId and subNodeId
    ids->pubNodeId = UA_NODEID_NULL;
    ids->grpNodeId   = UA_NODEID_NULL;
    ids->readGroupId = UA_NODEID_NULL;
    ids->readerId = UA_NODEID_NULL;
    ids->dtsNodeId = UA_NODEID_NULL;
    ids->publisherId = 0;
    ids->groupId = 0;
    ids->writerId = 0;
    ids->namespace = 0;
}
void Sub_IdCollection_init(Sub_IdCollection *ids) {
    if(!ids) return;
    memset(ids, 0, sizeof(Sub_IdCollection));
    ids->conId       = UA_NODEID_NULL;
    ids->subNodeId   = UA_NODEID_NULL;
    ids->pubNodeId   = UA_NODEID_NULL;
    ids->grpNodeId   = UA_NODEID_NULL;
    ids->readGroupId = UA_NODEID_NULL;
    ids->readerId    = UA_NODEID_NULL;
    ids->dtsNodeId = UA_NODEID_NULL;
}
/* for debugging only */
char *Sub_IdCollection_toString(const Sub_IdCollection *ids) {
    if(!ids) return NULL;

    UA_String conIdStr     = UA_STRING_NULL;
    UA_String subNodeIdStr = UA_STRING_NULL;
    UA_String pubNodeIdStr = UA_STRING_NULL;
    UA_String dtsNodeStr = UA_STRING_NULL;
    UA_String grpNodeIdStr = UA_STRING_NULL;
    UA_String readGroupIdStr = UA_STRING_NULL;
    UA_String readerIdStr  = UA_STRING_NULL;

    if(!UA_NodeId_isNull(&ids->conId))
        UA_NodeId_print(&ids->conId, &conIdStr);
    if(!UA_NodeId_isNull(&ids->subNodeId))
        UA_NodeId_print(&ids->subNodeId, &subNodeIdStr);
    if(!UA_NodeId_isNull(&ids->pubNodeId))
        UA_NodeId_print(&ids->pubNodeId, &pubNodeIdStr);
    if(!UA_NodeId_isNull(&ids->grpNodeId))
        UA_NodeId_print(&ids->grpNodeId, &grpNodeIdStr);
    if(!UA_NodeId_isNull(&ids->readGroupId))
        UA_NodeId_print(&ids->readGroupId, &readGroupIdStr);
    if(!UA_NodeId_isNull(&ids->readerId))
        UA_NodeId_print(&ids->readerId, &readerIdStr);

    size_t bufSize = 512;
    char *buf = (char*)UA_malloc(bufSize);
    if(!buf) return NULL;

    snprintf(buf, bufSize,
         "Sub_IdCollection {\n"
         "  conId       = %.*s\n"
         "  subNodeId   = %.*s\n"
         "  pubNodeId   = %.*s\n"
         "  grpNodeId   = %.*s\n"
         "  readGroupId = %.*s\n"
         "  readerId    = %.*s\n"
         "  publisherId = %u\n"
         "  groupId     = %u\n"
         "  writerId    = %u\n"
         "  namespace   = %u\n"
         "}",
         (int)conIdStr.length,     conIdStr.data     ? (const char*)conIdStr.data     : "",
         (int)subNodeIdStr.length, subNodeIdStr.data ? (const char*)subNodeIdStr.data : "",
         (int)pubNodeIdStr.length, pubNodeIdStr.data ? (const char*)pubNodeIdStr.data : "",
         (int)grpNodeIdStr.length, grpNodeIdStr.data ? (const char*)grpNodeIdStr.data : "",
         (int)readGroupIdStr.length, readGroupIdStr.data ? (const char*)readGroupIdStr.data : "",
         (int)readerIdStr.length,  readerIdStr.data  ? (const char*)readerIdStr.data  : "",
         ids->publisherId,
         ids->groupId,
         ids->writerId,
         ids-> namespace);


    UA_String_clear(&conIdStr);
    UA_String_clear(&subNodeIdStr);
    UA_String_clear(&pubNodeIdStr);
    UA_String_clear(&readGroupIdStr);
    UA_String_clear(&readerIdStr);

    return buf;
}