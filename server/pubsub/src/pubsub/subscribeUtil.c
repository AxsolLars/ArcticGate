#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "configs.h"
#include "collection.h"
#include <unistd.h>
#include <open62541/util.h>

#define SIZE 2000000
#define BITS_PER_BYTE 8
#define ARRAY_SIZE ((SIZE + BITS_PER_BYTE - 1) / BITS_PER_BYTE)
uint8_t WriterIds[ARRAY_SIZE] = {0};
uint8_t WriterNodesIds[ARRAY_SIZE] = {0};

void set_bit(uint8_t IDs[], int index) {
    IDs[index / BITS_PER_BYTE] |=  (1 << (index % BITS_PER_BYTE));
}

void clear_bit(uint8_t IDs[], int index) {
    IDs[index / BITS_PER_BYTE] &= ~(1 << (index % BITS_PER_BYTE));
}

int get_bit(uint8_t IDs[], int index) {
    return (IDs[index / BITS_PER_BYTE] >> (index % BITS_PER_BYTE)) & 1;
}

int typeFromString(const char *typeStr) {
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

void fillFields(UA_DataSetMetaDataType *pMetaData, FieldConfig ** fieldConfs, size_t fieldCount) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->fieldsSize = fieldCount;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new(pMetaData->fieldsSize,
                            &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for (int i = 0; i < fieldCount; i++){
        FieldConfig * field = fieldConfs[i];
        UA_FieldMetaData_init(&pMetaData->fields[i]);

        pMetaData->fields[i].name = UA_STRING(field->name);
        int idx = typeFromString(field->type);
        if(idx < 0) {
            continue;
        }


        UA_NodeId_copy(&UA_TYPES[idx].typeId,
                       &pMetaData->fields[i].dataType);

        if(idx == UA_TYPES_BYTESTRING) {
            pMetaData->fields[i].builtInType = UA_NS0ID_BYTESTRING; 
        } else {
            pMetaData->fields[i].builtInType = (UA_Byte) idx;
        }


        pMetaData->fields[i].valueRank = -1;
    }
}

UA_StatusCode 
addValueAttributes(UA_Server * server, UA_DataSetReaderConfig * readerConfig, Sub_IdCollection * ids){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig->dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    // printf("FIELD ID: %d\n", (ids->writerId * 100));
    // fflush(stdout); 
    for(size_t i = 0; i < readerConfig->dataSetMetaData.fieldsSize; i++) {
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig->dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig->dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig->dataSetMetaData.fields[i].dataType;
            if(vAttr.dataType.identifier.numeric == UA_NS0ID_BYTESTRING) {
            UA_ByteString empty = UA_BYTESTRING_NULL;
            UA_Variant_setScalar(&vAttr.value, &empty, &UA_TYPES[UA_TYPES_BYTESTRING]);
        }
        UA_NodeId newNode;
        UA_UInt32 fieldId = (UA_UInt32) i + (ids->writerId * 100);
        
        // if (get_bit(WriterNodesIds ,fieldId)){
        //     printf("MISTAKE MISTAKE MISTAKE MISTKAE for WriterIDID: %d\n", ids->dtsNodeId.identifier.numeric);
        //     printf("PUBID %d\n", ids->publisherId);
        //     printf("GROUPID %d\n", ids->groupId);
        //     printf("WRITERID %d\n", ids->writerId);
        //     fflush(stdout);
        //     sleep(2);
        // }
        // set_bit(WriterNodesIds, fieldId);
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(ids->namespace, fieldId),
                                           ids->grpNodeId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(ids->namespace, (char *)readerConfig->dataSetMetaData.fields[i].name.data),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);

        
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(server, ids->readerId,
                                                           readerConfig->dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig->dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    return retval;
}

UA_StatusCode
addDataSetReader(UA_Server *server, Sub_IdCollection * ids, char * dataSetName, UA_DataSetReaderConfig * readerConfig) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    char buf[32];
    snprintf(buf, sizeof(buf), "Group %u", ids->groupId);
    readerConfig->name = UA_STRING_ALLOC(buf);

    printf("%u", ids->publisherId);
    UA_Variant_setScalarCopy(&readerConfig->publisherId,
                         &ids->publisherId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    readerConfig->writerGroupId    = ids->groupId;
    readerConfig->dataSetWriterId  = ids->writerId;
    UA_UadpDataSetReaderMessageDataType *msgSettings = UA_UadpDataSetReaderMessageDataType_new();
    msgSettings->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
    UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
    UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER |
    UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID;
    readerConfig->messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    readerConfig->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    readerConfig->messageSettings.content.decoded.data = msgSettings;
    retval |= UA_Server_addDataSetReader(server, ids->readGroupId, readerConfig,
                                         &ids->readerId);
    UA_UadpDataSetReaderMessageDataType_delete(msgSettings);
    return retval;
}


UA_StatusCode
addReaderGroup(UA_Server *server, Sub_IdCollection * ids) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Group %u", ids->groupId);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));

    readerGroupConfig.name = UA_STRING_ALLOC(buffer);

    retval |= UA_Server_addReaderGroup(server, ids->conId, &readerGroupConfig,
                                       &ids->readGroupId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "UA add reader Group failed: 0x%08x", retval);
        return retval;
    }
    retval |= UA_Server_setReaderGroupOperational(server, ids->readGroupId);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "UA setting reader Group operational failed: 0x%08x", retval);
    }
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", buffer);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(ids->namespace, buffer);

    UA_NodeId groupFolderId;
    retval |= UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                      ids->pubNodeId, 
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                      browseName,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                      oAttr, NULL, &groupFolderId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "UA add Object Node failed: 0x%08x", retval);
        return retval;
    }

    ids->grpNodeId = groupFolderId;
    return retval;
}

UA_StatusCode 
addPublisherObject(UA_Server *server, Sub_IdCollection * ids, char * folderName){
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Adds an Object on the Server for the Publisher*/
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", folderName);
    ids->namespace = UA_Server_addNamespace(server, folderName);

    folderBrowseName = UA_QUALIFIEDNAME (ids->namespace, folderName);

    return UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             ids->subNodeId,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &ids->pubNodeId);
}

UA_StatusCode
addSubscribedVariables (UA_Server *server, Sub_IdCollection * ids) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

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
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &ids->subNodeId);
    return retval;
}


UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl, Sub_IdCollection * ids) {
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
    UA_UInt16 pubId = UA_UInt16_random();
    UA_Variant_setScalarCopy(&connectionConfig.publisherId, &pubId,
                            &UA_TYPES[UA_TYPES_UINT16]);
    
    
    /* Sets the ConnectionId*/
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &ids->conId);

    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}
