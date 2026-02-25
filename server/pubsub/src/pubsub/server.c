#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/util.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "configs.h"
#include "collection.h"
#include "nodeIdMap.h"
#include "server.h"
#include "serverUtils.h"

UA_StatusCode 
Server_addDataset(UA_Server *server, DataSetConfig *dataSetConf, ServerContext * ctx){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    char buffer[128];  
    snprintf(buffer, sizeof(buffer),
            "/group/%u/%s",
            ctx->identifiers.groupId,
            dataSetConf->name);

    ctx->config.topic = strdup(buffer);

    if(!dataSetConf) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: dataSetConf is NULL!");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Starting Dataset '%s' (WriterID: %u)", 
                dataSetConf->name ? dataSetConf->name : "NULL", dataSetConf->writerId);

    ctx->identifiers.writerId = dataSetConf->writerId;
    
    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Initializing MetaData for %d fields", dataSetConf->fieldCount);
    Util_initializeDataSetMetaData(&readerConfig, dataSetConf->fieldCount);

    // Allocate targetVars
    UA_FieldTargetDataType *targetVars =
    (UA_FieldTargetDataType *)UA_calloc(
        dataSetConf->fieldCount,
        sizeof(UA_FieldTargetDataType));
    
    if(!targetVars && dataSetConf->fieldCount > 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Allocation for targetVars failed!");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    retval |= Util_addPublishedDataSet(server, ctx, dataSetConf->name);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Add PublishedDataSet failed");
        UA_free(targetVars);
        return retval;
    }

    // Field Loop
    for(int i = 0; i < dataSetConf->fieldCount; i++) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 1: Grabbing field config...", i);
        FieldConfig * field = dataSetConf->fields[i];
        
        if(!field) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Field is NULL!", i);
            continue;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 2: Calling Util_addVariableNode for '%s'", i, field->name);
        retval |= Util_addVariableNode(server, ctx, field->name, field->type);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 3: Calling Util_initializeDataSetFieldConfig", i);
        UA_DataSetFieldConfig dsfConf;
        Util_initializeDataSetFieldConfig(&dsfConf, field->name, field->type, ctx);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 4: Calling Util_addDataSetField", i);
        retval |= Util_addDataSetField(server, ctx, &dsfConf);
        
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 5: Calling Util_addDataSetMetaData", i);
        retval |= Util_addDataSetMetaData(&readerConfig, field->name, field->type, i);
        
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 6: Calling Util_addTargetVariable", i);
        Util_addTargetVariable(targetVars, ctx, i);
        
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: [Loop %d] Point 7: Iteration success", i);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Loop finished. Initializing ReaderConfig...");

    Util_initializeReaderConfig(&readerConfig, ctx, dataSetConf->name);
    retval |= Util_addDataSetReader(server, ctx, &readerConfig);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Creating Target Variables...");
    retval |= UA_Server_DataSetReader_createTargetVariables(server, ctx->nodes.readerId,
                                                           dataSetConf->fieldCount, targetVars);

    // Memory Cleanup for targetVars
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Cleaning targetVars...");
    for(size_t i = 0; i < dataSetConf->fieldCount; i++) {
     UA_FieldTargetDataType_clear(&targetVars[i]);
    }
    UA_free(targetVars); // Use UA_free to match UA_calloc

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Adding DataSetWriter...");
    retval |= Util_addDataSetWriter(server, ctx, dataSetConf->name);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DS: Dataset '%s' complete.", dataSetConf->name);
    return retval;
}

UA_StatusCode
Server_addGroup(UA_Server *server, WriterGroupConfig * groupConf, ServerContext * ctx, NodeIdMapEntry ** writerGroupIdMap){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    if (!groupConf) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: groupConf is NULL!");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Processing Group '%s' (ID: %u)", 
                groupConf->name ? groupConf->name : "NULL", groupConf->groupId);

    
    ctx->identifiers.groupId = groupConf->groupId;


    char buffer[64];
    snprintf(buffer, sizeof(buffer), "/group/%u", ctx->identifiers.groupId);

    ctx->config.topic = strdup(buffer);

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Calling Util_addReaderGroup...");
    retval |= Util_addReaderGroup(server, ctx);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "addReaderGroup failed: 0x%08x", retval);
        return retval;
    }

    // Map Lookup
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Searching map for WriterGroupId: %u", groupConf->groupId);
    UA_NodeId * foundWriterGroupId = findNodeInMap(groupConf->groupId, *writerGroupIdMap);

    if (foundWriterGroupId == NULL) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: WriterGroup not in map. Creating new one...");
        retval |= Util_addWriterGroup(server, ctx, groupConf->interval, groupConf->name);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "addWriterGroup failed: 0x%08x", retval);
            return retval;
        }
        
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Adding new NodeId to map...");
        addNodeIdToMap(groupConf->groupId, &ctx->nodes.writerGroupId, writerGroupIdMap);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: WriterGroup found in map. Reusing NodeId.");
        // WARNING: Shpallow copy might cause issues if the original NodeId is cleared!
        ctx->nodes.writerGroupId = *foundWriterGroupId;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Starting DataSet loop (%d datasets)", groupConf->dataSetCount);

    // iterates over all the datasets
    for(int i = 0; i < groupConf->dataSetCount; i++){
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: -> Entering DataSet [%d]", i);
        
        if (groupConf->dataSets[i] == NULL) {
             UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: DataSet [%d] is NULL!", i);
             return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval |= Server_addDataset(server, groupConf->dataSets[i], ctx);
        
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server_addDataset [%d] failed: 0x%08x", i, retval);
            return retval;
        }
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: -> DataSet [%d] finished.", i);
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Group: Finished Group '%s'", groupConf->name);
    return retval;
}

UA_StatusCode 
Server_addPublisher(UA_Server *server, PublisherConfig * pubConf, ServerContext * ctx, NodeIdMapEntry ** writerGroupIdMap){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    if(!pubConf) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server_addPublisher: pubConf is NULL!");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: Processing Publisher '%s' (ID: %u)", 
                pubConf->name ? pubConf->name : "NULL", pubConf->publisherId);

    // Adds the current publisher Id to the ServerContext
    ctx->identifiers.publisherId = pubConf->publisherId;

    // Adds the current publisher Object to the Server
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: Calling Util_addPublisherObject...");
    retval |= Util_addPublisherObject(server, ctx, pubConf->name);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "addPublisherObject failed: 0x%08x", retval);
        return retval;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: Object added. Iterating %d WriterGroups...", pubConf->groupCount);

    // Iterates over all the writer Groups to add them to the server
    for(int i = 0; i < pubConf->groupCount; i++){
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: -> Entering Group [%d]", i);
        
        if(pubConf->writerGroups[i] == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: Group [%d] configuration is NULL!", i);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval |= Server_addGroup(server, pubConf->writerGroups[i], ctx, writerGroupIdMap);
        
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Server_addGroup [%d] failed: 0x%08x", i, retval);
            return retval;
        }
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: -> Group [%d] added successfully", i);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pub: Finished all groups for '%s'", pubConf->name);
    
    return retval; // Added missing return
}

UA_StatusCode 
Server_addSysConfig(UA_Server *server, SystemConfig * sysConf, ServerContext* ctx){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Starting Subscribed Variables...");
    retval |= Util_addSubscribedVariables(server, ctx);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "addSubscribedVariables failed: 0x%08x", retval);
        return retval;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Subscribed Variables added successfully.");

    NodeIdMapEntry * writerGroupIdMap = NULL;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Entering Publisher loop. Count: %d", sysConf->publisherCount);

    for(int i = 0; i < sysConf->publisherCount; i++){
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--- Processing Publisher [%d] ---", i);
        
        if(sysConf->publishers[i] == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Publisher [%d] is NULL pointer!", i);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval |= Server_addPublisher(server, sysConf->publishers[i], ctx, &writerGroupIdMap);
        
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Server_addPublisher [%d] failed: 0x%08x", i, retval);
            return retval;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Publisher [%d] added. Clearing ServerContext...", i);
        
        // Critical Section: If it crashes here, ServerContextClear has a memory bug
        ServerContextClear(ctx);
        
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: ServerContext cleared for Publisher [%d].", i);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: All publishers processed. Clearing WriterGroup Map...");
    
    // Critical Section: If it crashes here, your clearMap (uthash logic) is the culprit
    clearMap(&writerGroupIdMap);
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AddSysConfig: Map cleared. System configuration complete.");
    
    return retval;
}

int runServer(  
    UA_String* subscribeTransportProfile, 
    UA_NetworkAddressUrlDataType*  subscribeNetworkAddressUrl, 
    UA_String* publishTransportProfile, 
    UA_NetworkAddressUrlDataType*  publishNetworkAddressUrl, 
    SystemConfig* sysConf, 
    bool debug,
    ServerConfig serverConfig){
        UA_StatusCode retval = UA_STATUSCODE_GOOD;
        // Add server
        UA_Server *server = UA_Server_new();
        UA_ServerConfig *config = UA_Server_getConfig(server);
        //Set Loglevel
        static UA_Logger logger;
        if (debug){
            
            logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_TRACE);
            
        } else {
            logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_WARNING);
        }
        config->logging = &logger;
        // Create Id collection -> dynamically changing
        ServerContext * ctx = malloc(sizeof(ServerContext));
        ServerContextInit(ctx);
        ctx->config = serverConfig;
        // Add connection for Subscribing
        retval |= Util_addSubConnection(server, subscribeTransportProfile, subscribeNetworkAddressUrl, ctx);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "addSubConnection failed: 0x%08x", retval);
            return retval;
        }
        // Add connection for Publishing
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "MQTT flag set to %s", ctx->config.useMqtt ? "true" : "false" );
        if(ctx->config.useMqtt){
            retval |= Util_addMqttPubConnection(server, publishNetworkAddressUrl, ctx);
        } else {
            retval |= Util_addUdpPubConnection(server, publishTransportProfile, publishNetworkAddressUrl, ctx);
        }
        
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "addPubConnection failed: 0x%08x", retval);
            return retval;
        }
        
        // Start Recursive construction with Systemstemconfig
        retval |= Server_addSysConfig(server, sysConf, ctx);
        
        if (retval != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SysConfig failed");
            return EXIT_FAILURE;
        } else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SERVER STARTING...");
            retval = UA_Server_enableAllPubSubComponents(server);
            if (retval != UA_STATUSCODE_GOOD){
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enabling all PubSub Components failed: 0x%08x", retval);
            } else {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Enabling all PubSub Components succeded");
            }
            retval |= UA_Server_runUntilInterrupt(server);
        }
        
        free(ctx);
        UA_Server_delete(server);
        return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
    }