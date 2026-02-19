#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/util.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "configs.h"
#include "subscribeUtil.h"
#include "collection.h"

UA_StatusCode 
Sub_addDataset(UA_Server *server, DataSetConfig * dataSetConf, Sub_IdCollection * ids ){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    ids->writerId = dataSetConf->writerId;
    FieldConfig ** fields = dataSetConf->fields;

    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    
    fillFields(&readerConfig.dataSetMetaData, fields, dataSetConf->fieldCount);
    retval |= addDataSetReader(server,  ids, dataSetConf->name, &readerConfig);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "addDataSetReader failed: 0x%08x", retval);
        return retval;
    }
    retval |= addValueAttributes(server, &readerConfig, ids);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "addValueAttributes failed: 0x%08x", retval);
        return retval;
    }
    return retval;
}
UA_StatusCode
Sub_addWriterGroup(UA_Server *server, WriterGroupConfig * groupConf, Sub_IdCollection * ids){
    UA_StatusCode retval;
    ids->groupId = groupConf->groupId;
    retval = addReaderGroup(server, ids);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "addReaderGroup failed: 0x%08x", retval);
        return retval;
    }
    DataSetConfig ** dataSets = groupConf->dataSets;
    for (int i = 0; i < groupConf->dataSetCount; i++){
        // sleep(1);
        // printf("dataSetId %u with ids:\n", dataSets[i]->writerId);
        // char * s = IdCollection_toString(ids);
        // printf("%s\n", s);
        // UA_free(s);
        // fflush(stdout);
        retval |= Sub_addDataset(server, dataSets[i], ids);
    }
    return retval;
}

UA_StatusCode 
Sub_addPublisher(UA_Server *server, PublisherConfig * pubConf, Sub_IdCollection * ids){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    ids->publisherId = 10;
    addPublisherObject(server, ids, pubConf->name);
    WriterGroupConfig ** writerGroups = pubConf->writerGroups;
    for (int i = 0; i < pubConf->groupCount; i++){
        /*
        printf("GroupId %u with ids:\n", writerGroups[i]->groupId);
        char * s = IdCollection_toString(ids);
        printf("%s\n", s);
        UA_free(s);
        fflush(stdout);
        */
        retval |= Sub_addWriterGroup(server, writerGroups[i], ids);
    }
    return retval;
}

UA_StatusCode Sub_addSysConfig(UA_Server *server, SystemConfig * sysConf, Sub_IdCollection * ids){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= addSubscribedVariables(server, ids);

    PublisherConfig ** publishers = sysConf->publishers;
    
    for(int i = 0; i < sysConf->publisherCount; i++){
        /*
        printf("PubId %u with ids:\n", publishers[i]->publisherId);
        fflush(stdout);
        char * s = IdCollection_toString(ids);
        printf("%s\n", s);
        UA_free(s);
        fflush(stdout);
        */
        retval |= Sub_addPublisher(server, publishers[i], ids);
        Sub_IdCollection_clear(ids);
    }
    return retval;
}

int runSubscribe(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl, SystemConfig* sysConf, bool debug) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Set log level to DEBUG (prints everything) */
    static UA_Logger logger;
    if (debug){
        
        logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_DEBUG);
        
    } else {
        logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_WARNING);
    }
    config->logging = &logger;
    /* Initializes the Sub_IdCollection*/
    Sub_IdCollection * ids = malloc(sizeof(Sub_IdCollection));
    Sub_IdCollection_init(ids);

    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl, ids);
    if (retval != UA_STATUSCODE_GOOD){
        return EXIT_FAILURE;
    }

    retval |= Sub_addSysConfig(server, sysConf, ids);

    if (retval != UA_STATUSCODE_GOOD){
        return EXIT_FAILURE;
    }

    retval = UA_Server_runUntilInterrupt(server);
    free(ids);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}