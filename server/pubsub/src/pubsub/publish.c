#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "publishUtil.h"
#include "collection.h"
#include "configs.h"



UA_StatusCode Pub_addDataset(UA_Server *server,
                             DataSetConfig *dataSetConf,   
                             Pub_IdCollection *ids) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    ids->writerId = dataSetConf->writerId;

    retval = Pub_addPublishedDataSet(server, ids, dataSetConf->name);
    
    retval = Pub_fillFields(server, ids, dataSetConf->fields, dataSetConf->fieldCount);
    
    retval = Pub_addDataSetWriter(server, ids, dataSetConf->name);

    return retval;
}

UA_StatusCode Pub_addGroup(UA_Server *server,
                                 WriterGroupConfig *groupConf,
                                 Pub_IdCollection *ids) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    ids->groupId = groupConf->groupId;

    
    retval |= Pub_addWriterGroup(server, ids, groupConf->interval, groupConf->name);

    for (size_t i = 0; i < groupConf->dataSetCount; i++) {
        retval |= Pub_addDataset(server, groupConf->dataSets[i], ids);
    }

    return retval;
}


UA_StatusCode 
Pub_addConf(UA_Server *server, PublisherConfig * pubConf, Pub_IdCollection * ids){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    ids->publisherId = pubConf->publisherId;
    WriterGroupConfig ** writerGroups = pubConf->writerGroups;
    for (int i = 0; i < pubConf->groupCount; i++){

        retval |= Pub_addGroup(server, writerGroups[i], ids);
    }
    return retval;
}

int runPublish(UA_String *transportProfile,
               UA_NetworkAddressUrlDataType *networkAddressUrl,
               SystemConfig* sysConf,
                int publisherIndex,
                bool debug) {
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


    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    Pub_IdCollection * ids = malloc(sizeof(Pub_IdCollection)); 
    Pub_IdCollection_init(ids);

    PublisherConfig * publisher = sysConf->publishers[publisherIndex];
    ids->publisherId = publisher->publisherId;

    retval |= Pub_addPubSubConnection(server, transportProfile, networkAddressUrl, ids, publisher->name);
    
    retval |= Pub_addConf(server, publisher, ids);

    UA_ServerConfig *configs = UA_Server_getConfig(server);

    retval |= UA_Server_runUntilInterrupt(server);

    free(ids);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}