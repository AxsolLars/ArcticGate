#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#include "collection.h"
UA_StatusCode
Pub_addDataSetWriter(UA_Server * server, Pub_IdCollection * ids, char* name);
UA_StatusCode
Pub_fillFields(UA_Server *server, Pub_IdCollection * ids, FieldConfig ** fields, int fieldCount);
UA_StatusCode
Pub_addPublishedDataSet(UA_Server *server, Pub_IdCollection * ids, char * datasetName);
UA_StatusCode
Pub_addWriterGroup(UA_Server *server, Pub_IdCollection * ids, int interval, char * groupName);
UA_StatusCode
Pub_addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl, Pub_IdCollection * ids, char * publisherName);

                    
#endif