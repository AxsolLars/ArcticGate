#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#include "collection.h"

static void
updateVariable(UA_Server *server, void *data) {
    UA_NodeId *varNode = (UA_NodeId*)data;

    /* Example: generate a random float */
    UA_Float value = (UA_Float)((float)rand() / RAND_MAX * 100.0f);

    UA_Variant val;
    UA_Variant_setScalar(&val, &value, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_Server_writeValue(server, *varNode, val);
}

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
Pub_addDataSetWriter(UA_Server * server, Pub_IdCollection * ids, char* name){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_DataSetWriterConfig dswConf;
    memset(&dswConf, 0, sizeof(dswConf));
    dswConf.name = UA_STRING(name);
    dswConf.dataSetWriterId = ids->writerId;

    UA_NodeId dswId;
    retval |= UA_Server_addDataSetWriter(server,ids->writerGroupId, ids->publishedDataSetId, &dswConf, &dswId);
    if (retval != UA_STATUSCODE_GOOD) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Adding DataSetWriter failed (PubId=%u, GroupId=%u, WriterId=%u): 0x%08x (%s)",
                 ids->publisherId,
                 ids->groupId,
                 ids->writerId,
                 retval,
                 UA_StatusCode_name(retval));
    return retval;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "DataSetWriter added successfully "
                    "(PubId=%u, GroupId=%u, WriterId=%u, NodeId ns=%u;i=%u)",
                    ids->publisherId,
                    ids->groupId,
                    dswConf.dataSetWriterId,
                    dswId.namespaceIndex,
                    dswId.identifier.numeric);
}
    return retval;
}

void
initializeDataSetFieldConfig(UA_DataSetFieldConfig * dsfConf, Pub_IdCollection *ids, FieldConfig * field){
    memset(dsfConf, 0, sizeof(*dsfConf));
        dsfConf->dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dsfConf->field.variable.fieldNameAlias = UA_STRING(field->name);
        dsfConf->field.variable.publishParameters.publishedVariable = ids->fieldNodeId;
        dsfConf->field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
}
void
initializeCallback(UA_Server * server, Pub_IdCollection *ids){
    UA_NodeId *varCopy = UA_NodeId_new();
    UA_NodeId_copy(&ids->fieldNodeId, varCopy);

    UA_Server_addRepeatedCallback(server, updateVariable, varCopy, 1000, &ids->callbackId);
}
void
initializeFieldNode(UA_Server *server, Pub_IdCollection *ids, FieldConfig * field){
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.displayName = UA_LOCALIZEDTEXT("en-US", field->name);

        int idx = typeFromString(field->type);
        vAttr.dataType = UA_TYPES[idx].typeId;
        UA_Server_addVariableNode(server,
            UA_NODEID_STRING(ids->namespace, field->name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(ids->namespace, field->name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            vAttr, NULL, &ids->fieldNodeId);
}

UA_StatusCode
Pub_fillFields(UA_Server *server, Pub_IdCollection * ids, FieldConfig ** fields, int fieldCount){
    
    for (size_t i = 0; i < fieldCount; i++) {

        initializeFieldNode(server, ids, fields[i]);
        
        initializeCallback(server, ids);

        UA_DataSetFieldConfig dsfConf;
        initializeDataSetFieldConfig(&dsfConf, ids, fields[i]);
        

        UA_NodeId dsfId;
        UA_Server_addDataSetField(server, ids->publishedDataSetId, &dsfConf, &dsfId);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
Pub_addPublishedDataSet(UA_Server *server, Pub_IdCollection * ids, char * datasetName){
    UA_PublishedDataSetConfig pdsConf;
    memset(&pdsConf, 0, sizeof(pdsConf));
    pdsConf.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConf.name = UA_STRING(datasetName);

    UA_AddPublishedDataSetResult result = UA_Server_addPublishedDataSet(server, &pdsConf, &ids->publishedDataSetId);
    UA_StatusCode retval = result.addResult;
    if (retval != UA_STATUSCODE_GOOD) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Adding PublishedDataSet failed: 0x%08x (%s)",
                 retval, UA_StatusCode_name(retval));
    return retval;
}

return retval;
}


void
Pub_initializeWriterGroupMessage(UA_WriterGroupConfig * writerGroupConfig, UA_UadpWriterGroupMessageDataType *writerGroupMessage){
    writerGroupMessage->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
    UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
    UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID;
    writerGroupConfig->messageSettings.content.decoded.data = writerGroupMessage;
}
void
Pub_initializeWriterGroupConfig(UA_WriterGroupConfig* writerGroupConfig, Pub_IdCollection * ids, int interval, char * groupName){
    memset(writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));

    writerGroupConfig->name = UA_String_fromChars(groupName);
    writerGroupConfig->publishingInterval = interval;
    writerGroupConfig->enabled = UA_TRUE;
    writerGroupConfig->writerGroupId = ids->groupId;
    writerGroupConfig->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig->messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
}

UA_StatusCode
Pub_addWriterGroup(UA_Server *server, Pub_IdCollection * ids, int interval, char * groupName) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;


    UA_WriterGroupConfig writerGroupConfig;
    Pub_initializeWriterGroupConfig(&writerGroupConfig, ids, interval, groupName);


    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    Pub_initializeWriterGroupMessage(&writerGroupConfig, writerGroupMessage);


    retval = UA_Server_addWriterGroup(server, ids->conId, &writerGroupConfig, &ids->writerGroupId);

    if (retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Adding WriterGroup failed: 0x%08x (StatusCode %s)",
                 retval, UA_StatusCode_name(retval));
        return retval;
    } else {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "WriterGroup added successfully "
                "(PubId=%u, GroupId=%u, NodeId ns=%u;i=%u)",
                ids->publisherId,
                ids->groupId,
                ids->writerGroupId.namespaceIndex,
                ids->writerGroupId.identifier.numeric);
}


    retval = UA_Server_setWriterGroupOperational(server, ids->writerGroupId);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Setting WriterGroup (ns=%u; i=%u) operational failed: 0x%08x (%s)",
                    ids->writerGroupId.namespaceIndex,
                    ids->writerGroupId.identifier.numeric,
                    retval,
                    UA_StatusCode_name(retval));
        return retval;
    }


    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    return retval;
}

UA_StatusCode
Pub_addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl, Pub_IdCollection * ids, char * publisherName) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(publisherName);
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    
    UA_UInt16 pubId = (UA_UInt16)ids->publisherId;
    UA_Variant_setScalarCopy(&connectionConfig.publisherId, &pubId,
                            &UA_TYPES[UA_TYPES_UINT16]);
    
    printf("publisherId %u\n", connectionConfig.publisherId);
    return UA_Server_addPubSubConnection (server, &connectionConfig, &ids->conId);
}