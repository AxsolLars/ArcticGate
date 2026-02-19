#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#include "collection.h"

static int typeFromString(const char *typeStr) {
    if(strcmp(typeStr, "UInt16") == 0) return UA_TYPES_UINT16;
    if(strcmp(typeStr, "UInt32") == 0) return UA_TYPES_UINT32;
    if(strcmp(typeStr, "Float")  == 0) return UA_TYPES_FLOAT;
    if(strcmp(typeStr, "Double") == 0) return UA_TYPES_DOUBLE;
    if(strcmp(typeStr, "String") == 0) return UA_TYPES_STRING;
    if(strcmp(typeStr, "Byte")   == 0) return UA_TYPES_BYTE;
    if(strcmp(typeStr, "DateTime") == 0) return UA_TYPES_DATETIME;
    if(strcmp(typeStr, "Int32")  == 0) return UA_TYPES_INT32;
    if(strcmp(typeStr, "Real")   == 0) return UA_TYPES_FLOAT;
    if(strcmp(typeStr, "ByteString")   == 0) return UA_TYPES_BYTESTRING;
    return -1;
}

UA_StatusCode
Util_addPublishedDataSet(UA_Server *server, ServerContext * ctx, char * datasetName){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // build and populate publishedDataSetConfig
    UA_PublishedDataSetConfig pdsConf;
    memset(&pdsConf, 0, sizeof(pdsConf));

    pdsConf.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConf.name = UA_STRING(datasetName);

    // Add publishedDataSet to server and isolate its id into the id collection
    UA_AddPublishedDataSetResult result = UA_Server_addPublishedDataSet(server, &pdsConf, &ctx->nodes.publishedDataSetId);
    retval |= result.addResult;

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Adding ServerPublishedDataSet failed: 0x%08x (%s)",
                    retval, UA_StatusCode_name(retval));
        return retval;
    }
    return retval;
}

UA_StatusCode
Util_addVariableNode(UA_Server * server, ServerContext * ctx, char * fieldname, char * type){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    int idx = typeFromString(type);

    // initialize value attributes and populate them. 
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", fieldname);
    vAttr.dataType = UA_TYPES[idx].typeId;


    retval |= UA_Server_addVariableNode(server, UA_NODEID_STRING(ctx->identifiers.nsIndex, fieldname),
                                           ctx->nodes.grpNodeId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(ctx->identifiers.nsIndex, fieldname),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &ctx->nodes.fieldNodeId);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "ServerAddVariableNode failed: 0x%08x", retval);
        return retval;
    }

    return retval;
}

void
Util_initializeDataSetFieldConfig(UA_DataSetFieldConfig * dsfConf, char * fieldname, char * type, ServerContext *ctx){
    memset(dsfConf, 0, sizeof(*dsfConf));
    dsfConf->dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsfConf->field.variable.fieldNameAlias = UA_STRING(fieldname);
    dsfConf->field.variable.publishParameters.publishedVariable = ctx->nodes.fieldNodeId;
    dsfConf->field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
}

UA_StatusCode
Util_addDataSetField(UA_Server * server, ServerContext * ctx, UA_DataSetFieldConfig * dsfConf){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId dsfId;
    UA_DataSetFieldResult result = UA_Server_addDataSetField(server, ctx->nodes.publishedDataSetId, dsfConf, &dsfId);
    
    retval |= result.result;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "ServerAddDataSetField failed: 0x%08x", retval);
        return retval;
    }

    return retval;
}

void
Util_addTargetVariable(UA_FieldTargetVariable  * targetVars, ServerContext * ctx, size_t index){
    UA_FieldTargetDataType_init(&targetVars[index].targetVariable);
    targetVars[index].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[index].targetVariable.targetNodeId = ctx->nodes.fieldNodeId;
}

void 
Util_initializeDataSetMetaData(UA_DataSetReaderConfig * readerConfig, size_t fieldCount){
    UA_DataSetMetaDataType *pMetaData = &readerConfig->dataSetMetaData;
    
    pMetaData->fieldsSize = fieldCount;
    
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new(pMetaData->fieldsSize,
                            &UA_TYPES[UA_TYPES_FIELDMETADATA]);
                            
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                 "MetaData Init: Allocated %zu fields at %p", fieldCount, (void*)pMetaData->fields);
}
UA_StatusCode
Util_addDataSetMetaData(UA_DataSetReaderConfig * readerConfig, char * fieldname, char * type, int currentIndex){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    // 1. Get a pointer to the existing metadata, do NOT copy the struct
    UA_DataSetMetaDataType *pMetaData = &readerConfig->dataSetMetaData;

    // 2. Safety check: Ensure the fields array exists
    if (pMetaData->fieldsSize <= (size_t)currentIndex || pMetaData->fields == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "MetaData Error: fields array is NULL or too small!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // 3. Initialize the specific field using the pointer
    UA_FieldMetaData_init(&pMetaData->fields[currentIndex]);

    // 4. Assign the name (UA_STRING_ALLOC is safer if fieldname is a local variable elsewhere)
    pMetaData->fields[currentIndex].name = UA_STRING_ALLOC(fieldname);

    int idx = typeFromString(type);
    
    // 5. Check if idx is valid for the UA_TYPES array to avoid out-of-bounds crash
    if (idx < 0 || idx >= UA_TYPES_COUNT) {
         UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown type: %s", type);
         return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    retval |= UA_NodeId_copy(&UA_TYPES[idx].typeId, &pMetaData->fields[currentIndex].dataType);
    
    if(idx == UA_TYPES_BYTESTRING) {
        pMetaData->fields[currentIndex].builtInType = UA_NS0ID_BYTESTRING; 
    } else {
        // Use the index directly from the UA_TYPES table
        pMetaData->fields[currentIndex].builtInType = (UA_Byte) UA_TYPES[idx].typeKind;
    }

    pMetaData->fields[currentIndex].valueRank = UA_VALUERANK_SCALAR; // -1
    return retval;
}

void
Util_initializeReaderConfig(UA_DataSetReaderConfig * readerConfig, ServerContext * ctx, char * name){
    char buf[32];
    snprintf(buf, sizeof(buf), "Group %u", ctx->identifiers.groupId);
    readerConfig->name = UA_STRING_ALLOC(buf);

    UA_Variant_setScalarCopy(&readerConfig->publisherId,
                         &ctx->identifiers.publisherId,
                         &UA_TYPES[UA_TYPES_UINT32]);
        
    readerConfig->writerGroupId    = ctx->identifiers.groupId;
    readerConfig->dataSetWriterId  = ctx->identifiers.writerId;
    UA_UadpDataSetReaderMessageDataType *msgSettings = UA_UadpDataSetReaderMessageDataType_new();
    msgSettings->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
    UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
    UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID;
    readerConfig->messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    readerConfig->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    readerConfig->messageSettings.content.decoded.data = msgSettings;
}

UA_StatusCode
Util_addDataSetReader(UA_Server *server, ServerContext * ctx, UA_DataSetReaderConfig * readerConfig){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Reader: Adding DataSetReader to ReaderGroup (NodeId ns=%u;i=%u)", 
                ctx->nodes.readGroupId.namespaceIndex, ctx->nodes.readGroupId.identifier.numeric);

    // Deep check of the config before calling the server
    if(!readerConfig) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reader: readerConfig is NULL!");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                 "Reader: MetaData FieldCount: %zu", readerConfig->dataSetMetaData.fieldsSize);

    retval = UA_Server_addDataSetReader(server, ctx->nodes.readGroupId, readerConfig,
                                         &ctx->nodes.readerId);
    
    if (retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Reader: UA_Server_addDataSetReader failed: 0x%08x", retval);
        return retval;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Reader: Successfully added Reader (NodeId ns=%u;i=%u)", 
                ctx->nodes.readerId.namespaceIndex, ctx->nodes.readerId.identifier.numeric);

    // Cleaning up the local config structure
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reader: Clearing readerConfig...");
    UA_DataSetReaderConfig_clear(readerConfig);
    
    return retval;
}

UA_StatusCode
initializeBrokerTransportSettings(){

}
UA_StatusCode
Util_addDataSetWriter(UA_Server * server, ServerContext * ctx, char* name){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_DataSetWriterConfig dswConf;
    memset(&dswConf, 0, sizeof(dswConf));
    dswConf.name = UA_STRING(name);
    dswConf.dataSetWriterId = ctx->identifiers.writerId;

    UA_NodeId dswId;
    retval |= UA_Server_addDataSetWriter(server,ctx->nodes.writerGroupId, ctx->nodes.publishedDataSetId, &dswConf, &dswId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Adding DataSetWriter failed (PubId=%u, GroupId=%u, WriterId=%u): 0x%08x (%s)",
                    ctx->identifiers.publisherId,
                    ctx->identifiers.groupId,
                    ctx->identifiers.writerId,
                    retval,
                    UA_StatusCode_name(retval));
        return retval;
    }
    return retval;
}

UA_StatusCode
Util_addReaderGroup(UA_Server *server, ServerContext * ctx) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Group %u", ctx->identifiers.groupId);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));

    readerGroupConfig.name = UA_STRING_ALLOC(buffer);

    retval |= UA_Server_addReaderGroup(server, ctx->nodes.subConId, &readerGroupConfig,
                                       &ctx->nodes.readGroupId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "UA add reader Group failed: 0x%08x", retval);
        return retval;
    }
    retval |= UA_Server_setReaderGroupOperational(server, ctx->nodes.readGroupId);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "UA setting reader Group operational failed: 0x%08x", retval);
    }
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", buffer);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(ctx->identifiers.nsIndex, buffer);

    UA_NodeId groupFolderId;
    retval |= UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                      ctx->nodes.pubNodeId, 
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                      browseName,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                      oAttr, NULL, &groupFolderId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "UA add Object Node failed: 0x%08x", retval);
        return retval;
    }

    ctx->nodes.grpNodeId = groupFolderId;
    return retval;
}

void
initializeWriterGroupMessage(UA_WriterGroupConfig * writerGroupConfig, UA_UadpWriterGroupMessageDataType *writerGroupMessage){
    writerGroupMessage->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
    UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
    UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID;
    writerGroupConfig->messageSettings.content.decoded.data = writerGroupMessage;
}
void
initializeWriterGroupConfig(UA_WriterGroupConfig* writerGroupConfig, ServerContext * ctx, int interval, char * groupName){
    memset(writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));

    writerGroupConfig->name = UA_String_fromChars(groupName);
    writerGroupConfig->publishingInterval = interval;
    writerGroupConfig->enabled = UA_TRUE;
    writerGroupConfig->writerGroupId = ctx->identifiers.groupId;
    writerGroupConfig->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig->messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
}

UA_StatusCode
Util_addWriterGroup(UA_Server *server, ServerContext * ctx, int interval, char * groupName) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;


    UA_WriterGroupConfig writerGroupConfig;
    initializeWriterGroupConfig(&writerGroupConfig, ctx, interval, groupName);


    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    initializeWriterGroupMessage(&writerGroupConfig, writerGroupMessage);


    retval = UA_Server_addWriterGroup(server, ctx->nodes.pubConId, &writerGroupConfig, &ctx->nodes.writerGroupId);

    if (retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Adding WriterGroup failed: 0x%08x (StatusCode %s)",
                 retval, UA_StatusCode_name(retval));
        return retval;
    } else {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "WriterGroup added successfully "
                "(PubId=%u, GroupId=%u, NodeId ns=%u;i=%u)",
                ctx->identifiers.publisherId,
                ctx->identifiers.groupId,
                ctx->nodes.writerGroupId.namespaceIndex,
                ctx->nodes.writerGroupId.identifier.numeric);
}


    retval = UA_Server_setWriterGroupOperational(server, ctx->nodes.writerGroupId);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Setting WriterGroup (ns=%u; i=%u) operational failed: 0x%08x (%s)",
                    ctx->nodes.writerGroupId.namespaceIndex,
                    ctx->nodes.writerGroupId.identifier.numeric,
                    retval,
                    UA_StatusCode_name(retval));
        return retval;
    }


    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    return retval;
}

UA_StatusCode 
Util_addPublisherObject(UA_Server *server, ServerContext * ctx, char * folderName){
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Adds an Object on the Server for the Publisher*/
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", folderName);
    ctx->identifiers.nsIndex = UA_Server_addNamespace(server, folderName);

    folderBrowseName = UA_QUALIFIEDNAME (ctx->identifiers.nsIndex, folderName);

    return UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             ctx->nodes.subNodeId,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &ctx->nodes.pubNodeId);
}

UA_StatusCode
Util_addSubscribedVariables (UA_Server *server, ServerContext * ctx) {
    /*Adds an Object for the subscribedVariables*/
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
    folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");

    /* takes out the subNodeId for the publishers to connect to*/
    retval |= UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &ctx->nodes.subNodeId);
    return retval;
}

UA_StatusCode
Util_addSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl, ServerContext * ctx) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*Creating conifugration for the connection*/
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    
    /* On Subscriber side the publisherId doesn't matter at first*/
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    
    /* Sets the ConnectionId*/
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &ctx->nodes.subConId);

    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

UA_StatusCode
Util_addPubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl, ServerContext * ctx)  {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("publisher");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = 10;
    
    printf("publisherId %u\n", connectionConfig.publisherId.uint32);
    return UA_Server_addPubSubConnection (server, &connectionConfig, &ctx->nodes.pubConId);

}
