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
    if(strcmp(typeStr, "Long") == 0)   return  UA_TYPES_INT64;
    if(strcmp(typeStr, "DateTime") == 0) return UA_TYPES_DATETIME;
    if(strcmp(typeStr, "Int32")  == 0) return UA_TYPES_INT32;
    if(strcmp(typeStr, "Real")   == 0) return UA_TYPES_FLOAT;
    if(strcmp(typeStr, "ByteString")   == 0) return UA_TYPES_BYTESTRING;
    if(strcmp(typeStr, "Bool" ) == 0) return UA_TYPES_BYTESTRING;
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
Util_addTargetVariable(UA_FieldTargetDataType   * targetVars, ServerContext * ctx, size_t index){
    UA_FieldTargetDataType_init(&targetVars[index]);
    targetVars[index].attributeId = UA_ATTRIBUTEID_VALUE;
    targetVars[index].targetNodeId = ctx->nodes.fieldNodeId;
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
    
    UA_DataSetMetaDataType *pMetaData = &readerConfig->dataSetMetaData;

    if (pMetaData->fieldsSize <= (size_t)currentIndex || pMetaData->fields == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "MetaData Error: fields array is NULL or too small!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FieldMetaData_init(&pMetaData->fields[currentIndex]);

    pMetaData->fields[currentIndex].name = UA_STRING_ALLOC(fieldname);

    int idx = typeFromString(type);
    
    if (idx < 0 || idx >= UA_TYPES_COUNT) {
         UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown type: %s", type);
         return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    retval |= UA_NodeId_copy(&UA_TYPES[idx].typeId, &pMetaData->fields[currentIndex].dataType);
    
    if(idx == UA_TYPES_BYTESTRING) {
        pMetaData->fields[currentIndex].builtInType = UA_NS0ID_BYTESTRING; 
    } else {
        pMetaData->fields[currentIndex].builtInType = (UA_Byte) UA_TYPES[idx].typeKind;
    }

    pMetaData->fields[currentIndex].valueRank = UA_VALUERANK_SCALAR;
    return retval;
}

void
Util_initializeReaderConfig(UA_DataSetReaderConfig * readerConfig, ServerContext * ctx, char * name){
    char buf[32];
    snprintf(buf, sizeof(buf), "Group %u", ctx->identifiers.groupId);
    readerConfig->name = UA_STRING_ALLOC(buf);
    readerConfig->publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig->publisherId.id.uint16 = ctx->identifiers.publisherId;
    readerConfig->writerGroupId    = ctx->identifiers.groupId;
    readerConfig->dataSetWriterId  = ctx->identifiers.writerId;

    UA_UadpDataSetReaderMessageDataType *msgSettings = UA_UadpDataSetReaderMessageDataType_new();
    msgSettings->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
    UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID;
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


    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Reader: Clearing readerConfig...");
    UA_DataSetReaderConfig_clear(readerConfig);
    
    return retval;
}

void
Util_addDataSetWriter(UA_Server *server, ServerContext * ctx, char * name) {
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING(name);
    dataSetWriterConfig.dataSetWriterId = ctx->identifiers.writerId;


    UA_JsonDataSetWriterMessageDataType jsonDswMd;
    UA_ExtensionObject messageSettings;
    if(ctx->config.useJson) {
        jsonDswMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
            (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID);

        messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
        messageSettings.content.decoded.data = &jsonDswMd;
    
        dataSetWriterConfig.messageSettings = messageSettings;
    }
    UA_BrokerDataSetWriterTransportDataType *dswTs =
        UA_BrokerDataSetWriterTransportDataType_new();
    UA_BrokerDataSetWriterTransportDataType_init(dswTs);

    dswTs->queueName = UA_STRING_ALLOC(ctx->config.topic);
    dswTs->metaDataQueueName = UA_STRING_ALLOC(ctx->config.metaQueueName);
    dswTs->metaDataUpdateTime = ctx->config.metaUpdateTime;
    dswTs->requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    UA_ExtensionObject transportSettings;
    UA_ExtensionObject_init(&transportSettings);
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_BROKERDATASETWRITERTRANSPORTDATATYPE];
    transportSettings.content.decoded.data = dswTs;
    

    dataSetWriterConfig.transportSettings = transportSettings;
    UA_Server_addDataSetWriter(server, ctx->nodes.writerGroupId, ctx->nodes.publishedDataSetId,
                               &dataSetWriterConfig, &dataSetWriterIdent);
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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
            "ReaderGroup added: ns=%u;i=%u",
            ctx->nodes.readGroupId.namespaceIndex,
            ctx->nodes.readGroupId.identifier.numeric);
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
    UA_ReaderGroupConfig_clear(&readerGroupConfig);
    ctx->nodes.grpNodeId = groupFolderId;
    return retval;
}

UA_StatusCode
Util_addWriterGroup(UA_Server *server, ServerContext *ctx, int interval, char * groupName) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Now we create a new WriterGroupConfig and add the group to the existing PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING(groupName);
    writerGroupConfig.publishingInterval = interval;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = ctx->identifiers.groupId;
    UA_UadpWriterGroupMessageDataType *writerGroupMessage;
    UA_JsonWriterGroupMessageDataType *Json_writerGroupMessage;

    if(ctx->config.useJson) {
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;

        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */
        Json_writerGroupMessage = UA_JsonWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * DataSetMessageHeader, SingleDataSetMessage and DataSetClassId in PayloadHeader
         * of NetworkMessage */
        Json_writerGroupMessage->networkMessageContentMask =0 ;
    
        writerGroupConfig.messageSettings.content.decoded.data = Json_writerGroupMessage;
    }

    else
    {
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */
        writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
         * of NetworkMessage */
        
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    }

    UA_BrokerWriterGroupTransportDataType *brokerTransportSettings =
        UA_BrokerWriterGroupTransportDataType_new();
    UA_BrokerWriterGroupTransportDataType_init(brokerTransportSettings);

    brokerTransportSettings->queueName = UA_STRING_ALLOC(ctx->config.topic);
    brokerTransportSettings->requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    UA_ExtensionObject transportSettings;
    UA_ExtensionObject_init(&transportSettings);
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings.content.decoded.data = brokerTransportSettings;

    writerGroupConfig.transportSettings = transportSettings;
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Util_addWriterGroup: pubConId (ns=%u;i=%u)",
                 ctx->nodes.pubConId.namespaceIndex,
                 ctx->nodes.pubConId.identifier.numeric);
    retval = UA_Server_addWriterGroup(server, ctx->nodes.pubConId, &writerGroupConfig, &ctx->nodes.writerGroupId);


    if (ctx->config.useJson) {
        UA_JsonWriterGroupMessageDataType_delete(Json_writerGroupMessage);
    }

    if (!ctx->config.useJson && writerGroupMessage) {
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
            "Topic: %.*s",
            (int)brokerTransportSettings->queueName.length,
            brokerTransportSettings->queueName.data);
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
    UA_UInt16 publisherIdentifier = (UA_UInt16)(UA_UInt32_random() & 0xFFFFu);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = publisherIdentifier;
    
    /* Sets the ConnectionId*/
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &ctx->nodes.subConId);

    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

UA_StatusCode
Util_addUdpPubConnection(UA_Server *server, UA_String *transportProfile,
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

    
    UA_UInt16 pubId = (UA_UInt16)(UA_UInt32_random() & 0xFFFFu);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = pubId;
    
    printf("publisherId %u\n", connectionConfig.publisherId);
    return UA_Server_addPubSubConnection (server, &connectionConfig, &ctx->nodes.pubConId);

}

UA_StatusCode
Util_addMqttPubConnection(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrl, ServerContext * ctx){
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating MQTT PubSub connection...");

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("MQTTpublisher");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
            "MQTT URL used by open62541: '%.*s'",
            (int)networkAddressUrl->url.length, networkAddressUrl->url.data);
    /* Determine Transport Profile */
    if(ctx->config.useJson) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Using JSON transport profile.");
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");
    } else {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Using UADP transport profile.");
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    }
    
    connectionConfig.enabled = true;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    /* Configure MQTT Client ID */
    UA_KeyValuePair connectionOptions[3];
    UA_String mqttClientId = UA_STRING(ctx->config.mqttClientId);
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "mqttClientId");
    UA_Variant_setScalar(&connectionOptions[0].value, &mqttClientId,
                     &UA_TYPES[UA_TYPES_STRING]);
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "MQTT Client ID: %.*s", 
                 (int)mqttClientId.length, mqttClientId.data);

    UA_UInt32 sendBuf = 65536;
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "sendBufferSize");
    UA_Variant_setScalar(&connectionOptions[1].value, &sendBuf,
                        &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 rcvBuf = 65536;
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "rcvBufferSize");
    UA_Variant_setScalar(&connectionOptions[2].value, &rcvBuf,
                        &UA_TYPES[UA_TYPES_UINT32]);

    UA_UInt16 pubId = (UA_UInt16)(UA_UInt32_random() & 0xFFFFu);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = pubId;
    
    
                        
    connectionConfig.connectionProperties.map = connectionOptions;
    connectionConfig.connectionProperties.mapSize = 3;

    /* Add the connection to the server */
    UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &ctx->nodes.pubConId);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to add PubSub connection. Error: %s", UA_StatusCode_name(retval));
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "MQTT PubSub connection added successfully. ID: %u", ctx->nodes.pubConId.identifier.numeric);
    }

    return retval;
}
